#include<stdio.h>
#include"eval.h"

int main(int argc, char **argv)
{
   if( argc != 2 )
      return printf("%s {input.txt}\n", *argv);

   FILE *infile = fopen(argv[1], "rb");
   if( infile == nullptr )
      return printf("Error opening %s\n", argv[1]);

   Node::Expr expr = Parse(infile);
   fclose(infile);

   std::shared_ptr<Node> r = Eval(&expr);
   r->Print(stdout);
   putchar('\n');
   return 0;
}
