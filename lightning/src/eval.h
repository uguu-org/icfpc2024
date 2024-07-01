#ifndef EVAL_H_
#define EVAL_H_

#include<inttypes.h>
#include<stdio.h>
#include<stdlib.h>
#include<map>
#include<memory>
#include<string>
#include<vector>

enum class NodeType
{
   Boolean,
   Integer,
   String,
   UnaryOp,
   BinaryOp,
   IfOp,
   Lambda,
   Variable,

   BinaryOpCurry1,
   IfOpCurry1,
   IfOpCurry2,
   LambdaCurry1,

   EndOfExpr
};

using IntType = int64_t;

struct Node
{
   using Expr = std::vector<std::shared_ptr<Node>>;
   using Environment = std::map<int, std::vector<Expr::iterator>>;

   Node() = default;
   virtual ~Node() = default;
   virtual int Arity() const = 0;
   virtual void Print(FILE *o) const = 0;

   virtual std::shared_ptr<Node> Eval(Node::Expr::iterator *arg,
                                      Environment *env)
   {
      fputs("Unexpected call to Eval for ", stderr);
      Print(stderr);
      fputc('\n', stderr);
      exit(EXIT_FAILURE);
      return nullptr;
   }

   NodeType type;
};

struct EndOfExprNode : public Node
{
   virtual int Arity() const { return -1; }
   virtual void Print(FILE *o) const final { fputs("EOF ", o); }
};

// T or F
struct BooleanNode : public Node
{
   int Arity() const final { return 0; }
   void Print(FILE *o) const final { fprintf(o, value ? "T " : "F "); }
   bool value;
};

// I {base94}
struct IntegerNode : public Node
{
   int Arity() const final { return 0; }
   void Print(FILE *o) const final { fprintf(o, "%" PRId64 " ", value); }
   IntType value;
};

// S {shifted text}
struct StringNode : public Node
{
   int Arity() const final { return 0; }

   void Print(FILE *o) const final
   {
      static constexpr char kDict[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!\"#$%&'()*+,-./:;<=>?@[\\]^_`|~ \n";
      fprintf(o, "%s=\"", value.c_str());
      for(int c : value)
         fputc(kDict[c - 33], o);
      fprintf(o, "\" ");
   }

   std::string value;
};

// U {argument}
struct UnaryNode : public Node
{
   int Arity() const final { return 1; }
   void Print(FILE *o) const final { fprintf(o, "U%c ", op); }
   std::shared_ptr<Node> Eval(Node::Expr::iterator *arg,
                              Node::Environment *env) final;

   char op;
};

// B {argument}
struct BinaryNode : public Node
{
   int Arity() const final { return 2; }
   void Print(FILE *o) const final { fprintf(o, "B%c ", op); }
   std::shared_ptr<Node> Eval(Node::Expr::iterator *arg,
                              Node::Environment *env) final;

   char op;
};
struct BinaryCurry1Node : public Node
{
   int Arity() const final { return 1; }

   void Print(FILE *o) const final
   {
      fprintf(o, "B%c ", op);
      (*arg1)->Print(o);
   }

   std::shared_ptr<Node> Eval(Node::Expr::iterator *arg,
                              Node::Environment *env) final;

   char op;
   Node::Expr::iterator arg1;
};

// ? {condition} {true_value} {false_value}
struct IfNode : public Node
{
   int Arity() const final { return 3; }
   void Print(FILE *o) const final { fprintf(o, "? "); }
   std::shared_ptr<Node> Eval(Node::Expr::iterator *arg,
                              Node::Environment *env) final;
};

struct IfCurry1Node : public Node
{
   int Arity() const final { return 2; }
   void Print(FILE *o) const final
   {
      fprintf(o, "? ");
      (*cond)->Print(o);
   }

   std::shared_ptr<Node> Eval(Node::Expr::iterator *arg,
                              Node::Environment *env) final;

   Node::Expr::iterator cond;
};

struct IfCurry2Node : public Node
{
   int Arity() const final { return 1; }
   void Print(FILE *o) const final
   {
      fprintf(o, "? ");
      (*cond)->Print(o);
      (*arg1)->Print(o);
   }

   std::shared_ptr<Node> Eval(Node::Expr::iterator *arg,
                              Node::Environment *env) final;

   Node::Expr::iterator cond;
   Node::Expr::iterator arg1;
};

// L {number}
struct LambdaNode : public Node
{
   int Arity() const final { return 1; }
   void Print(FILE *o) const final { fprintf(o, "L%d ", index); }
   std::shared_ptr<Node> Eval(Node::Expr::iterator *arg,
                              Node::Environment *env) final;

   int index;
};

struct LambdaCurry1Node : public Node
{
   int Arity() const final { return 0; }

   void Print(FILE *o) const final
   {
      fprintf(o, "curry L%d ", index);
      (*expr)->Print(o);
   }

   std::shared_ptr<Node> Eval(Node::Expr::iterator *arg,
                              Node::Environment *env) final;

   int index;
   Node::Expr::iterator expr;
};

// v {number}
struct VariableNode : public Node
{
   int Arity() const final { return 0; }
   void Print(FILE *o) const final { fprintf(o, "v%d ", index); }

   std::shared_ptr<Node> Call(Node::Environment *env);

   int index;
};

Node::Expr Parse(FILE *input);
std::shared_ptr<Node> Eval(Node::Expr *expr);

#endif  // EVAL_H_
