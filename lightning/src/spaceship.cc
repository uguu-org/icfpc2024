#include<stdio.h>
#include<stdlib.h>
#include<set>
#include<string>
#include<utility>

namespace {

using PointSet = std::set<std::pair<int, int>>;

static PointSet Load(int id)
{
   const std::string filename = "s_data/s" + std::to_string(id) + ".txt";
   FILE *infile = fopen(filename.c_str(), "rb");
   if( infile == nullptr )
   {
      fprintf(stderr, "Error reading %s\n", filename.c_str());
      exit(EXIT_FAILURE);
   }

   PointSet s;
   int x, y;
   while( fscanf(infile, "%d %d", &x, &y) == 2 )
      s.insert(std::make_pair(x, y));
   return s;
}

static void Solve(PointSet *input, std::string *output)
{
   struct Key
   {
      int dx, dy;
      char k;
   };
   static constexpr Key kKeys[9] =
   {
      {-1, +1, '7'}, { 0, +1, '8'}, {+1, +1, '9'},
      {-1,  0, '4'}, { 0,  0, '5'}, {+1,  0, '6'},
      {-1, -1, '1'}, { 0, -1, '2'}, {+1, -1, '3'},
   };

   int x = 0, y = 0, vx = 0, vy = 0;

   while( !input->empty() )
   {
      // Greedily check if an adjustment allows any point to be reached.
      int i = 0;
      for(; i < 9; i++)
      {
         PointSet::iterator f = input->find(
            std::make_pair(x + vx + kKeys[i].dx, y + vy + kKeys[i].dy));
         if( f != input->end() )
         {
            input->erase(f);
            output->push_back(kKeys[i].k);
            vx += kKeys[i].dx;
            vy += kKeys[i].dy;
            break;
         }
      }
      if( i >= 9 )
      {
         fprintf(stderr, "No trivial solution available, s=%s, x=%d, y=%d, vx=%d, y=%d\n", output->c_str(), x, y, vx, vy);
         exit(EXIT_FAILURE);
      }
      x += vx;
      y += vy;
   }
}

}  // namespace

int main(int argc, char **argv)
{
   if( argc != 2 )
      return printf("%s {id}\n", *argv);

   int id;
   if( sscanf(argv[1], "%d", &id) != 1 )
      return printf("Bad ID %s\n", argv[1]);

   PointSet points = Load(id);
   std::string solution;
   Solve(&points, &solution);
   printf("solve spaceship%d %s\n", id, solution.c_str());
   return 0;
}
