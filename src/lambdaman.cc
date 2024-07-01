#include<stdio.h>
#include<stdlib.h>

#include<algorithm>
#include<fstream>
#include<queue>
#include<string>
#include<vector>

namespace {

using MapData = std::vector<std::string>;

static constexpr char kWall = '#';
static constexpr char kDot = '.';
static constexpr char kEaten = '$';

static MapData Load(int id)
{
   const std::string filename = "msg/decoded/get_lambdaman_" + std::to_string(id) + ".txt";
   std::ifstream infile(filename);
   if( !infile.is_open() )
   {
      fprintf(stderr, "Error reading %s\n", filename.c_str());
      exit(EXIT_FAILURE);
   }

   MapData data;
   std::string line;
   while( std::getline(infile, line) )
      data.push_back(line);
   infile.close();
   return data;
}

// Find initial location.
static std::pair<int, int> FindInitialLocation(const MapData &data)
{
   for(int y = 0; y < static_cast<int>(data.size()); y++)
   {
      const std::string::size_type x = data[y].find('L');
      if( x != std::string::npos )
         return std::make_pair(static_cast<int>(x), y);
   }
   return std::make_pair(-1, -1);
}

// Count number of dots reachable from a particular starting location.
static int CountReachable(const MapData &input, int x, int y)
{
   const int height = static_cast<int>(input.size());
   const int width = static_cast<int>(input.front().size());
   int count = 0;

   // Make a writable copy of input.
   MapData data = input;

   // Visit nodes with breadth-first expansion.
   std::queue<std::pair<int, int>> visit;
   visit.push(std::make_pair(x, y));
   while( !visit.empty() )
   {
      x = visit.front().first;
      y = visit.front().second;
      visit.pop();

      if( x < 0 || y < 0 || x >= width || y >= height || data[y][x] != kDot )
         continue;

      data[y][x] = kEaten;
      count++;
      visit.push(std::make_pair(x - 1, y));
      visit.push(std::make_pair(x + 1, y));
      visit.push(std::make_pair(x, y - 1));
      visit.push(std::make_pair(x, y + 1));
   }
   return count;
}

// Move towards the neighbor with fewest dots available.  We want the
// fewest because that would be the least amount of work to backtrack
// from later.
static char MoveTowardSmallestNeighbor(int cl, int cr, int cu, int cd)
{
   std::vector<std::pair<int, char>> candidates;
   if( cl > 0 )
      candidates.push_back(std::make_pair(cl, 'L'));
   if( cr > 0 )
      candidates.push_back(std::make_pair(cr, 'R'));
   if( cu > 0 )
      candidates.push_back(std::make_pair(cu, 'U'));
   if( cd > 0 )
      candidates.push_back(std::make_pair(cd, 'D'));
   std::sort(candidates.begin(), candidates.end());
   const auto d = candidates.front();
   return d.second;
}

// Find direction that would be the shortest way to reach the nearest
// dot, walking over eaten tiles.
static char MoveTowardDotPatch(const MapData &input, int x, int y)
{
   const int height = static_cast<int>(input.size());
   const int width = static_cast<int>(input.front().size());

   // Make a writable copy of input.
   MapData data = input;

   // Expand 4 ways one step.
   using XYDir = std::pair<std::pair<int, int>, char>;
   std::queue<XYDir> visit;
   visit.push(std::make_pair(std::make_pair(x - 1, y), 'L'));
   visit.push(std::make_pair(std::make_pair(x + 1, y), 'R'));
   visit.push(std::make_pair(std::make_pair(x, y - 1), 'U'));
   visit.push(std::make_pair(std::make_pair(x, y + 1), 'D'));
   while( !visit.empty() )
   {
      x = visit.front().first.first;
      y = visit.front().first.second;
      const char dir = visit.front().second;
      visit.pop();

      if( x < 0 || y < 0 || x >= width || y >= height )
         continue;
      if( data[y][x] == kDot )
         return dir;

      // Expand from current location, propagating the initial direction
      // that got us here.
      if( data[y][x] == kEaten )
      {
         data[y][x] = kWall;
         visit.push(std::make_pair(std::make_pair(x - 1, y), dir));
         visit.push(std::make_pair(std::make_pair(x + 1, y), dir));
         visit.push(std::make_pair(std::make_pair(x, y - 1), dir));
         visit.push(std::make_pair(std::make_pair(x, y + 1), dir));
      }
   }
   return 0;
}

// Apply move direction.
static void ApplyMove(char dir, MapData *input, int *x, int *y)
{
   switch( dir )
   {
      case 'L': --*x; break;
      case 'R': ++*x; break;
      case 'U': --*y; break;
      case 'D': ++*y; break;
      default:
         // Unreachable.
         break;
   }

   (*input)[*y][*x] = kEaten;
}

static void Solve(MapData *input, std::string *output)
{
   auto [x, y] = FindInitialLocation(*input);

   (*input)[y][x] = kEaten;
   for(;;)
   {
      // Count dots that are immediately available in each of the 4 directions.
      const int cl = CountReachable(*input, x - 1, y);
      const int cr = CountReachable(*input, x + 1, y);
      const int cu = CountReachable(*input, x, y - 1);
      const int cd = CountReachable(*input, x, y + 1);
      if( cl + cr + cu + cd != 0 )
      {
         const char d = MoveTowardSmallestNeighbor(cl, cr, cu, cd);
         ApplyMove(d, input, &x, &y);
         output->push_back(d);
         continue;
      }

      // No dots available, find a direction that will lead to the nearest
      // patch of dots.
      const char d = MoveTowardDotPatch(*input, x, y);
      if( d == 0 )
         return;  // No dots remaining.

      ApplyMove(d, input, &x, &y);
      output->push_back(d);
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

   MapData data = Load(id);
   std::string solution;
   Solve(&data, &solution);
   printf("solve lambdaman%d %s\n", id, solution.c_str());
   return 0;
}
