#include"eval.h"

namespace {

static constexpr bool kTraceEval = false;

// Forward declarations.
static std::shared_ptr<Node> EvalWithEnvironment(Node::Expr::iterator *i,
                                                 Node::Environment *env);

// Environment operations.
static void BindVariable(
   Node::Environment *env, int index, Node::Expr::iterator arg)
{
   auto p = env->insert(std::make_pair(index,
                                       std::vector<Node::Expr::iterator>()));
   p.first->second.push_back(arg);
}

static void UnbindVariable(Node::Environment *env, int index)
{
   /*XXX
   Node::Environment::iterator f = env->find(index);
   if( f != env->end() )
   {
      f->second.pop_back();
      if( f->second.empty() )
         env->erase(f);
   }
   XXX*/
}

// Parse a sequence of characters from input as base94.
static IntType ParseBase94(FILE *input)
{
   IntType value = 0;
   int c;

   while( (c = fgetc(input)) != EOF )
   {
      if( c < '!' || c > '~' )
         break;
      value = value * 94 + (c - 33);
   }
   return value;
}

// Parse a sequence of characters from input as a string.
static std::string ParseString(FILE *input)
{
   std::string value;
   int c;

   while( (c = fgetc(input)) != EOF )
   {
      if( c < '!' || c > '~' )
         break;
      value.push_back(c);
   }
   return value;
}

// Number conversions.
static IntType StrToInt(const std::string &s)
{
   IntType value = 0;
   for(int c : s)
      value = value * 94 + c - 33;
   return value;
}

static std::string IntToStr(IntType i)
{
   //               18446744073709551616
   char buffer[] = "!!!!!!!!!!!!!!!!!!!!!";
   char *p = buffer + sizeof(buffer) - 1;
   if( i <= 0 )
   {
      p--;
   }
   else
   {
      while( i != 0 )
      {
         --p;
         *p = (i % 94) + 33;
         i /= 94;
      }
   }
   return p;
}

// Create nodes.
static std::shared_ptr<Node> MakeEndOfExpr()
{
   EndOfExprNode *n = new EndOfExprNode;
   n->type = NodeType::EndOfExpr;
   return std::shared_ptr<Node>(n);
}

static std::shared_ptr<Node> MakeBoolean(bool value)
{
   BooleanNode *n = new BooleanNode;
   n->type = NodeType::Boolean;
   n->value = value;
   return std::shared_ptr<Node>(n);
}

static std::shared_ptr<Node> MakeInteger(IntType value)
{
   IntegerNode *n = new IntegerNode;
   n->type = NodeType::Integer;
   n->value = value;
   return std::shared_ptr<Node>(n);
}

static std::shared_ptr<Node> MakeString(std::string value)
{
   StringNode *n = new StringNode;
   n->type = NodeType::String;
   n->value.swap(value);
   return std::shared_ptr<Node>(n);
}

static std::shared_ptr<Node> MakeUnary(FILE *input)
{
   UnaryNode *n = new UnaryNode;
   n->type = NodeType::UnaryOp;
   n->op = fgetc(input);
   return std::shared_ptr<Node>(n);
}

static std::shared_ptr<Node> MakeBinary(FILE *input)
{
   BinaryNode *n = new BinaryNode;
   n->type = NodeType::BinaryOp;
   n->op = fgetc(input);
   return std::shared_ptr<Node>(n);
}

static std::shared_ptr<Node> MakeIf(FILE *input)
{
   IfNode *n = new IfNode;
   n->type = NodeType::IfOp;
   return std::shared_ptr<Node>(n);
}

static std::shared_ptr<Node> MakeLambda(FILE *input)
{
   LambdaNode *n = new LambdaNode;
   n->type = NodeType::Lambda;
   n->index = static_cast<int>(ParseBase94(input));
   return std::shared_ptr<Node>(n);
}

static std::shared_ptr<Node> MakeVariable(FILE *input)
{
   VariableNode *n = new VariableNode;
   n->type = NodeType::Variable;
   n->index = static_cast<int>(ParseBase94(input));
   return std::shared_ptr<Node>(n);
}

// Assert value to be a particular type.
static bool BoolValue(std::shared_ptr<Node> node)
{
   return dynamic_cast<BooleanNode*>(node.get())->value;
}
static IntType IntValue(std::shared_ptr<Node> node)
{
   return dynamic_cast<IntegerNode*>(node.get())->value;
}
static std::string StrValue(std::shared_ptr<Node> node)
{
   return dynamic_cast<StringNode*>(node.get())->value;
}

// Syntactic sugar.
static bool EvalBool(Node::Expr::iterator *arg, Node::Environment *env)
{
   return BoolValue(EvalWithEnvironment(arg, env));
}
static IntType EvalInt(Node::Expr::iterator *arg, Node::Environment *env)
{
   return IntValue(EvalWithEnvironment(arg, env));
}
static std::string EvalStr(Node::Expr::iterator *arg, Node::Environment *env)
{
   return StrValue(EvalWithEnvironment(arg, env));
}

// Skip to next token without evaluating current node.
static void AdvanceToNextNode(Node::Expr::iterator *i)
{
   switch( (**i)->type )
   {
      case NodeType::Boolean:
      case NodeType::Integer:
      case NodeType::String:
      case NodeType::Variable:
         ++*i;
         return;

      case NodeType::UnaryOp:
      case NodeType::Lambda:
         ++*i;
         AdvanceToNextNode(i);
         return;

      case NodeType::BinaryOp:
         ++*i;
         AdvanceToNextNode(i);
         AdvanceToNextNode(i);
         return;

      case NodeType::IfOp:
         ++*i;
         AdvanceToNextNode(i);
         AdvanceToNextNode(i);
         AdvanceToNextNode(i);
         return;

      case NodeType::BinaryOpCurry1:
      case NodeType::IfOpCurry1:
      case NodeType::IfOpCurry2:
      case NodeType::LambdaCurry1:
         fputs("Unexpected internal node\n", stderr);
         exit(EXIT_FAILURE);
         break;

      case NodeType::EndOfExpr:
         fputs("Unexpected end of expression\n", stderr);
         exit(EXIT_FAILURE);
         break;
   }
}

// Function application.
static std::shared_ptr<Node> Apply(Node::Expr::iterator arg1,
                                   Node::Expr::iterator *arg2,
                                   Node::Environment *env)
{
   if( (*arg1)->type == NodeType::Lambda )
   {
      LambdaNode *l = static_cast<LambdaNode*>(arg1->get());
      BindVariable(env, l->index, *arg2);
      ++arg1;
      std::shared_ptr<Node> r = EvalWithEnvironment(&arg1, env);
      UnbindVariable(env, l->index);
      return r;
   }

   std::shared_ptr<Node> x = EvalWithEnvironment(&arg1, env);
   return x->Eval(arg2, env);
}

// Recursive evaluator.
static std::shared_ptr<Node> EvalWithEnvironment(Node::Expr::iterator *i,
                                                 Node::Environment *env)
{
   std::shared_ptr<Node> op = **i;
   ++*i;

   if( kTraceEval )
   {
      fprintf(stderr, "Eval (arity %d): ", op->Arity());
      op->Print(stderr);
      fputc('\n', stderr);
   }

   switch( op->Arity() )
   {
      case 0:
         if( op->type == NodeType::Variable )
            return static_cast<VariableNode*>(op.get())->Call(env);
         return op;
      case 1:
         return op->Eval(i, env);
      case 2:
         {
            std::shared_ptr<Node> curry1 = op->Eval(i, env);
            return curry1->Eval(i, env);
         }
      case 3:
         {
            std::shared_ptr<Node> curry1 = op->Eval(i, env);
            std::shared_ptr<Node> curry2 = curry1->Eval(i, env);
            return curry2->Eval(i, env);
         }
      default:
         break;
   }

   fprintf(stderr, "Unexpected arity %d\n", op->Arity());
   exit(EXIT_FAILURE);
   return nullptr;
}

}  // namespace

Node::Expr Parse(FILE *input)
{
   Node::Expr r;

   int c;
   while( (c = fgetc(input)) != EOF )
   {
      switch( c )
      {
         case 'T': r.push_back(MakeBoolean(true)); break;
         case 'F': r.push_back(MakeBoolean(false)); break;
         case 'I': r.push_back(MakeInteger(ParseBase94(input))); break;
         case 'S': r.push_back(MakeString(ParseString(input))); break;
         case 'U': r.push_back(MakeUnary(input)); break;
         case 'B': r.push_back(MakeBinary(input)); break;
         case '?': r.push_back(MakeIf(input)); break;
         case 'L': r.push_back(MakeLambda(input)); break;
         case 'v': r.push_back(MakeVariable(input)); break;
         default: break;
      }
   }

   r.push_back(MakeEndOfExpr());
   return r;
}

std::shared_ptr<Node> UnaryNode::Eval(Node::Expr::iterator *arg,
                                      Node::Environment *env)
{
   switch( op )
   {
      case '-': return MakeInteger(-EvalInt(arg, env));
      case '!': return MakeBoolean(!EvalBool(arg, env));
      case '#': return MakeInteger(StrToInt(EvalStr(arg, env)));
      case '$': return MakeString(IntToStr(EvalInt(arg, env)));
      default:
         break;
   }

   fprintf(stderr, "Unsupported unary op %c\n", op);
   exit(EXIT_FAILURE);
   return nullptr;
}

std::shared_ptr<Node> BinaryNode::Eval(Node::Expr::iterator *arg,
                                       Node::Environment *env)
{
   BinaryCurry1Node *b = new BinaryCurry1Node;
   b->type = NodeType::BinaryOpCurry1;
   b->op = op;
   b->arg1 = *arg;
   AdvanceToNextNode(arg);
   return std::shared_ptr<Node>(b);
}

std::shared_ptr<Node> BinaryCurry1Node::Eval(Node::Expr::iterator *arg,
                                             Node::Environment *env)
{
   Node::Expr::iterator i_arg1 = arg1;
   switch( op )
   {
      case '+': return MakeInteger(EvalInt(&i_arg1, env) + EvalInt(arg, env));
      case '-': return MakeInteger(EvalInt(&i_arg1, env) - EvalInt(arg, env));
      case '*': return MakeInteger(EvalInt(&i_arg1, env) * EvalInt(arg, env));
      case '/':
         {
            const IntType ia = EvalInt(&i_arg1, env);
            const IntType ib = EvalInt(arg, env);
            if( ib == 0 )
            {
               fprintf(stderr, "Divide by zero: %" PRId64 " / %" PRId64 "\n", ia, ib);
               exit(EXIT_FAILURE);
            }
            return MakeInteger(ia / ib);
         }
      case '%':
         {
            const IntType ia = EvalInt(&i_arg1, env);
            const IntType ib = EvalInt(arg, env);
            if( ib == 0 )
            {
               fprintf(stderr, "Divide by zero: %" PRId64 " %% %" PRId64 "\n", ia, ib);
               exit(EXIT_FAILURE);
            }
            return MakeInteger(ia % ib);
         }
      case '<':
         return MakeBoolean(EvalInt(&i_arg1, env) < EvalInt(arg, env));
      case '>':
         return MakeBoolean(EvalInt(&i_arg1, env) > EvalInt(arg, env));
      case '=':
         {
            std::shared_ptr<Node> a = EvalWithEnvironment(&i_arg1, env);
            std::shared_ptr<Node> b = EvalWithEnvironment(arg, env);
            if( a->type != b->type )
            {
               fputs("Types mismatched for B=, a=", stderr);
               a->Print(stderr);
               fputs("b=", stderr);
               b->Print(stderr);
               fputc('\n', stderr);
               exit(EXIT_FAILURE);
            }
            switch( a->type )
            {
               case NodeType::Boolean:
                  return MakeBoolean(BoolValue(a) == BoolValue(b));
               case NodeType::Integer:
                  return MakeBoolean(IntValue(a) == IntValue(b));
               case NodeType::String:
                  return MakeBoolean(StrValue(a) == StrValue(b));
               default:
                  fputs("Unexpected expression type for first operand B=\n", stderr);
                  exit(EXIT_FAILURE);
            }
         }
         break;
      case '|':
         return MakeBoolean(EvalBool(&i_arg1, env) || EvalBool(arg, env));
      case '&':
         return MakeBoolean(EvalBool(&i_arg1, env) && EvalBool(arg, env));
      case '.':
         return MakeString(EvalStr(&i_arg1, env) + EvalStr(arg, env));
      case 'T':
         {
            const IntType x = EvalInt(&i_arg1, env);
            return MakeString(EvalStr(arg, env).substr(0, x));
         }
      case 'D':
         {
            const IntType x = EvalInt(&i_arg1, env);
            std::string y = EvalStr(arg, env);
            y.erase(0, x);
            return MakeString(y);
         }

      case '$':
         return Apply(i_arg1, arg, env);

      default:
         fprintf(stderr, "Unsupported binary op %c\n", op);
         exit(EXIT_FAILURE);
         break;
   }

   // Unreachable.
   return nullptr;
}

std::shared_ptr<Node> IfNode::Eval(Node::Expr::iterator *arg,
                                   Node::Environment *env)
{
   IfCurry1Node *i = new IfCurry1Node;
   i->type = NodeType::IfOpCurry1;
   i->cond = *arg;
   AdvanceToNextNode(arg);
   return std::shared_ptr<Node>(i);
}

std::shared_ptr<Node> IfCurry1Node::Eval(Node::Expr::iterator *arg,
                                         Node::Environment *env)
{
   IfCurry2Node *i = new IfCurry2Node;
   i->type = NodeType::IfOpCurry2;
   i->cond = cond;
   i->arg1 = *arg;
   AdvanceToNextNode(arg);
   return std::shared_ptr<Node>(i);
}

std::shared_ptr<Node> IfCurry2Node::Eval(Node::Expr::iterator *arg,
                                         Node::Environment *env)
{
   Node::Expr::iterator i_arg0 = cond;
   if( EvalBool(&i_arg0, env) )
   {
      Node::Expr::iterator i_arg1 = arg1;
      std::shared_ptr<Node> r = EvalWithEnvironment(&i_arg1, env);
      AdvanceToNextNode(arg);
      return r;
   }
   return EvalWithEnvironment(arg, env);
}

std::shared_ptr<Node> LambdaNode::Eval(Node::Expr::iterator *arg,
                                       Node::Environment *env)
{
   LambdaCurry1Node *l = new LambdaCurry1Node;
   l->type = NodeType::LambdaCurry1;
   l->index = index;
   l->expr = *arg;
   return std::shared_ptr<Node>(l);
}

std::shared_ptr<Node> LambdaCurry1Node::Eval(Node::Expr::iterator *arg,
                                             Node::Environment *env)
{
   BindVariable(env, index, *arg);
   Node::Expr::iterator i_expr = expr;
   std::shared_ptr<Node> r = EvalWithEnvironment(&i_expr, env);
   UnbindVariable(env, index);
   return r;
}

std::shared_ptr<Node> VariableNode::Call(Node::Environment *env)
{
   Node::Environment::iterator f = env->find(index);

   if( f != env->end() )
   {
      // Should this be the old environment of when the lambda was created?
      Node::Expr::iterator arg = f->second.back();
      return EvalWithEnvironment(&arg, env);
   }

   fprintf(stderr, "Unbound variable %d\n", index);
   exit(EXIT_FAILURE);
   return nullptr;
}

std::shared_ptr<Node> Eval(Node::Expr *expr)
{
   Node::Expr::iterator i = expr->begin();
   Node::Environment e;
   return EvalWithEnvironment(&i, &e);
}
