#include<ctype.h>
#include<gmpxx.h>
#include<inttypes.h>
#include<stdio.h>
#include<stdlib.h>

#include<map>
#include<memory>
#include<string>
#include<utility>
#include<vector>

namespace {

static constexpr int kMaxTicks = 100;
static constexpr bool kUseDiff = true;

using Cell = std::string;
using Row = std::vector<Cell>;
using Grid = std::vector<Row>;
struct Snapshot
{
   // Grid contents.
   Grid grid;

   // Amount of shift needed for the first snapshot to match the coordinates
   // of the current snapshot.  That is, if (u,v) is a cell in the initial
   // snapshot, (u+x,v+y) would be the corresponding cell in the final snapshot.
   int x = 0, y = 0;
};

using Timeline = std::vector<Snapshot>;

using XYList = std::vector<std::pair<int, int>>;

struct PendingWrites
{
   // List of (x,y) tuples marking cells to be deleted.
   //
   // During simulation step, the deletes are buffered until all writes
   // are done.  Logically, all removals happen before all writes, but
   // we don't know which cells to remove until we have attempted those
   // writes, so what we do is buffer the removals, and apply them later
   // if those same locations are not overwritten.
   XYList remove;

   // Set of ((x,y),value) tuples recording cells that have already been
   // written.
   //
   // Writes are actually committed right away, but we record which cells
   // are touched during the simulation step to check for write conflicts.
   std::map<std::pair<int, int>, std::string> add;
};

// Load initial grid.
static Grid Load(const char *filename)
{
   FILE *infile = fopen(filename, "rb");
   if( infile == nullptr )
   {
      fprintf(stderr, "Failed to open %s\n", filename);
      exit(EXIT_FAILURE);
   }

   Grid grid;
   bool first_line = true;
   for(int c = fgetc(infile); c != EOF; c = fgetc(infile))
   {
      if( c == '\n' )
      {
         grid.push_back(Row());
         first_line = false;
         continue;
      }
      if( first_line )
         continue;

      if( c == '-' )
      {
         grid.back().push_back("-");

         // Read ahead in case if it's a negative number.
         int next_c = fgetc(infile);
         if( next_c != EOF && isdigit(next_c) )
         {
            Cell &cell = grid.back().back();
            while( next_c != EOF && isdigit(next_c) )
            {
               cell.push_back(next_c);
               next_c = fgetc(infile);
            }
         }

         // Push the space or newline back.
         ungetc(next_c, infile);
         continue;
      }
      if( isdigit(c) )
      {
         grid.back().push_back(std::string(1, c));
         Cell &cell = grid.back().back();
         while( (c = fgetc(infile)) != EOF )
         {
            if( !isdigit(c) )
               break;
            cell.push_back(c);
         }

         // Push the space or newline back.
         ungetc(c, infile);
         continue;
      }

      // Operators.
      if( !isspace(c) )
      {
         // Empty cells are stored as empty strings, everything else
         // is copied as is.
         if( c == '.' )
            grid.back().push_back(Cell());
         else
            grid.back().push_back(std::string(1, c));
      }
   }
   fclose(infile);

   // Drop the final empty row.
   if( grid.back().empty() )
      grid.pop_back();

   // Make all rows the same size.
   size_t width = 0;
   for(const Row &row : grid)
      width = std::max(width, row.size());
   for(Row &row : grid)
      row.resize(width);

   return grid;
}

// Check if a particular XY pair is in the highlight list.
static bool ShouldHighlight(const XYList &highlights, int x, int y)
{
   for(const auto &[u, v] : highlights)
   {
      if( u == x && v == y )
         return true;
   }
   return false;
}

// Write grid state to stdout.
static void Dump(const Grid &grid, const XYList &highlights)
{
   const int height = static_cast<int>(grid.size());
   const int width = static_cast<int>(grid.front().size());
   for(int y = 0; y < height; y++)
   {
      for(int x = 0; x < width; x++)
      {
         const Cell &cell = grid[y][x];
         if( cell.empty() )
         {
            if( ShouldHighlight(highlights, x, y) )
               printf("\x1b[97;41m  .\x1b[0m");
            else
               printf("  .");
            continue;
         }

         if( ShouldHighlight(highlights, x, y) )
            printf("\x1b[97;41m %2s\x1b[0m", cell.c_str());
         else
            printf(" %2s", cell.c_str());
      }
      putchar('\n');
   }
}

static void DumpWithDiff(const Snapshot &s0, const Snapshot &s1)
{
   if( !kUseDiff )
   {
      Dump(s1.grid, {});
      return;
   }

   const Grid &t0 = s0.grid;
   const Grid &t1 = s1.grid;
   const int height0 = static_cast<int>(t0.size());
   const int width0 = static_cast<int>(t0.front().size());
   const int height = static_cast<int>(t1.size());
   const int width = static_cast<int>(t1.front().size());
   const int dx = s1.x - s0.x;
   const int dy = s1.y - s0.y;

   bool changed = false;
   for(int y = 0; y < height; y++)
   {
      const int y0 = y - dy;
      for(int x = 0; x < width; x++)
      {
         const int x0 = x - dx;
         const Cell &cell = t1[y][x];
         if( y0 >= 0 && y0 < height0 && x0 >= 0 && x0 < width0 &&
             t0[y0][x0] != cell )
         {
            changed = true;
            if( t0[y0][x0].empty() )
            {
               // Added value.
               printf(" \x1b[92m%2s\x1b[0m", cell.c_str());
            }
            else if( cell.empty() )
            {
               // Removed value.
               printf("  \x1b[91m.\x1b[0m");
            }
            else
            {
               // Modified value.
               printf(" \x1b[93m%2s\x1b[0m", cell.c_str());
            }
            continue;
         }

         // No change, or out of bounds.
         if( cell.empty() )
            printf("  .");
         else
            printf(" %2s", t1[y][x].c_str());
      }
      putchar('\n');
   }

   if( !changed && &s0 != &s1 )
   {
      puts("Reached steady state without submitting output");
      exit(EXIT_FAILURE);
   }
}

// Check input grid features.
static bool HasOutput(const Grid &grid)
{
   for(const Row &row : grid)
   {
      for(const Cell &cell : row)
      {
         if( !cell.empty() && cell[0] == 'S' )
            return true;
      }
   }
   return false;
}

// Replace cells with initial values.
static void AddInput(Grid &grid,
                     char identifier,
                     const char *value,
                     XYList *input_positions)
{
   const int height = static_cast<int>(grid.size());
   const int width = static_cast<int>(grid.front().size());
   for(int y = 0; y < height; y++)
   {
      for(int x = 0; x < width; x++)
      {
         if( !grid[y][x].empty() && grid[y][x][0] == identifier )
         {
            grid[y][x] = value;
            input_positions->push_back(std::make_pair(x, y));
         }
      }
   }
}

// Check if cell contains a binary operator.
static bool IsBinaryOp(const Cell &cell)
{
   return cell.size() == 1 &&
          (cell[0] == '+' ||
           cell[0] == '-' ||
           cell[0] == '*' ||
           cell[0] == '/' ||
           cell[0] == '%' ||
           cell[0] == '=' ||
           cell[0] == '#');
}

// Check if there are any operators that writes beyond the grid edges.
// If so, resize the final snapshot to fit and return true.
static bool ResizeUp(Timeline *timeline, int *timestamp)
{
   Snapshot &snapshot = (*timeline)[*timestamp];
   Grid &grid = snapshot.grid;
   if( grid.size() < 2 )
      return false;

   const Row &y0 = grid.front();
   const Row &y1 = grid[1];
   const int width = static_cast<int>(y0.size());
   for(int x = 0; x < width; x++)
   {
      if( y0[x][0] == '^' && !y1[x].empty() )
      {
         grid.insert(grid.begin(), Row(width, Cell()));
         snapshot.y++;
         return true;
      }
   }
   return false;
}

static bool ResizeDown(Timeline *timeline, int *timestamp)
{
   Snapshot &snapshot = (*timeline)[*timestamp];
   Grid &grid = snapshot.grid;
   if( grid.size() < 2 )
      return false;

   const Row &ry0 = grid.back();
   const Row &ry1 = grid[grid.size() - 2];
   const int width = static_cast<int>(ry0.size());
   for(int x = 0; x < width; x++)
   {
      if( ry1[x].empty() )
         continue;
      if( ry0[x][0] == 'v' ||
          (x > 0 && IsBinaryOp(ry0[x]) && !ry0[x - 1].empty()) )
      {
         grid.push_back(Row(width, Cell()));
         return true;
      }
   }
   return false;
}

static bool ResizeLeft(Timeline *timeline, int *timestamp)
{
   Snapshot &snapshot = (*timeline)[*timestamp];
   Grid &grid = snapshot.grid;
   const int height = static_cast<int>(grid.size());
   if( height < 2 )
      return false;
   const int width = static_cast<int>(grid.front().size());
   if( width < 2 )
      return false;
   for(int y = 0; y < height; y++)
   {
      if( grid[y][0][0] == '<' && !grid[y][1].empty() )
      {
         for(Row &r : grid)
            r.insert(r.begin(), Cell());
         snapshot.x++;
         return true;
      }
   }
   return false;
}

static bool ResizeRight(Timeline *timeline, int *timestamp)
{
   Snapshot &snapshot = (*timeline)[*timestamp];
   Grid &grid = snapshot.grid;
   const int height = static_cast<int>(grid.size());
   if( height < 2 )
      return false;
   const int width = static_cast<int>(grid.front().size());
   if( width < 2 )
      return false;
   for (int y = 0; y < height; y++)
   {
      if( grid[y][width - 2].empty() )
         continue;
      if( grid[y][width - 1][0] == '>' ||
          (y > 0 &&
           IsBinaryOp(grid[y][width - 1]) &&
           !grid[y - 1][width - 1].empty()))
      {
         for(Row &r : grid)
            r.push_back(Cell());
         return true;
      }
   }
   return false;
}

static bool ResizeTimeline(Timeline *timeline, int *timestamp)
{
   // Resize in all directions without short-circuit.
   const bool resize_up = ResizeUp(timeline, timestamp);
   const bool resize_left = ResizeLeft(timeline, timestamp);
   const bool resize_right = ResizeRight(timeline, timestamp);
   const bool resize_down = ResizeDown(timeline, timestamp);
   return resize_up || resize_down || resize_left || resize_right;
}

// Write to a particular cell, returns true if simulation is done.
static bool Write(Grid &grid,
                  int x,
                  int y,
                  const Cell &value,
                  PendingWrites *pending_writes,
                  std::string *output)
{
   auto p = pending_writes->add.insert(std::make_pair(std::make_pair(x, y),
                                                      value));
   if( !p.second && p.first->second != value )
   {
      printf("Conflicting writes to (%d,%d): %s vs %s\n",
             x, y, p.first->second.c_str(), value.c_str());
      Dump(grid, {{x, y}});
      exit(EXIT_FAILURE);
   }

   if( !grid[y][x].empty() && grid[y][x][0] == 'S' )
   {
      grid[y][x] = value;
      *output = value;
      return true;
   }

   grid[y][x] = value;
   return false;
}

// Read a generic value with bounds check.
static const std::string kEmptyString{};

static const std::string &Read(const Grid &grid, int x, int y)
{
   if( y < 0 ||
       x < 0 ||
       y >= static_cast<int>(grid.size()) ||
       x >= static_cast<int>(grid.front().size()) )
   {
      return kEmptyString;
   }
   return grid[y][x];
}

// Read number at a particular cell, returns NULL if there is no value.
static std::unique_ptr<mpz_class> ReadNumber(const Grid &grid, int x, int y)
{
   const std::string &cell = Read(grid, x, y);
   if( cell.empty() )
      return nullptr;

   if( cell[0] >= '0' && cell[0] <= '9' )
      return std::make_unique<mpz_class>(cell);
   if( cell[0] == '-' && cell.size() > 1 )
      return std::make_unique<mpz_class>(cell);
   return nullptr;
}

// Perform arithmetic at a particular cell.
// Returns true if simulation is done.
static bool Arith(const Grid &t0,
                  Grid &t1,
                  int x,
                  int y,
                  PendingWrites *pending_writes,
                  std::string *output)
{
   std::unique_ptr<mpz_class> a = ReadNumber(t0, x - 1, y);
   if( a == nullptr )
      return false;

   std::unique_ptr<mpz_class> b = ReadNumber(t0, x, y - 1);
   if( b == nullptr )
      return false;

   switch( t0[y][x][0] )
   {
      case '+': *a += *b; break;
      case '-': *a -= *b; break;
      case '*': *a *= *b; break;
      case '/': *a /= *b; break;
      case '%': *a %= *b; break;
      default:
         printf("Unsupported operator %s\n", t0[y][x].c_str());
         exit(EXIT_FAILURE);
         break;
   }

   const std::string result = a->get_str();
   const bool write_down = Write(t1, x, y + 1, result, pending_writes, output);
   const bool write_right = Write(t1, x + 1, y, result, pending_writes, output);
   pending_writes->remove.push_back(std::make_pair(x - 1, y));
   pending_writes->remove.push_back(std::make_pair(x, y - 1));
   return write_down || write_right;
}

// Perform a comparison.  Returns true if simulation is done.
static bool Compare(const Grid &t0,
                    Grid &t1,
                    int x,
                    int y,
                    PendingWrites *pending_writes,
                    std::string *output)
{
   const std::string &a = Read(t0, x - 1, y);
   if( a.empty() )
      return false;
   const std::string &b = Read(t0, x, y - 1);
   if( b.empty() )
      return false;

   switch( t0[y][x][0] )
   {
      case '=':
         if( a != b )
            return false;
         break;
      case '#':
         if( a == b )
            return false;
         break;
      default:
         printf("Unsupported comparison %s\n", t0[y][x].c_str());
         exit(EXIT_FAILURE);
         break;
   }

   const bool write_right = Write(t1, x + 1, y, b, pending_writes, output);
   const bool write_down = Write(t1, x, y + 1, a, pending_writes, output);
   pending_writes->remove.push_back(std::make_pair(x - 1, y));
   pending_writes->remove.push_back(std::make_pair(x, y - 1));
   return write_right || write_down;
}

// Perform a time warp operation, returns true if simulation is done.
//
// This function doesn't need to buffer pending writes, since it modifies
// a previous snapshot in time.
static bool TimeWarp(Timeline *timeline,
                     int *timestamp,
                     const Grid &t0,
                     int x,
                     int y,
                     int *new_timestamp,
                     std::string *output)
{
   const std::string &v = Read(t0, x, y - 1);
   if( v.empty() )
      return false;
   const std::string &s_dx = Read(t0, x - 1, y);
   if( s_dx.empty() )
      return false;
   const std::string &s_dy = Read(t0, x + 1, y);
   if( s_dy.empty() )
      return false;
   const std::string &s_dt = Read(t0, x, y + 1);
   if( s_dt.empty() )
      return false;

   // Get destination timestamp.
   const int dt = std::stoi(s_dt);
   const int target_t = *timestamp - 1 - dt;
   if( target_t < 0 )
   {
      printf("Attempting to warp before initial snapshot, dt=%d\n", dt);
      Dump(t0, {{x, y + 1}});
      exit(EXIT_FAILURE);
   }
   if( *new_timestamp >= 0 && *new_timestamp != target_t )
   {
      printf("Attempting to warp to different times: %d vs %d\n",
             *new_timestamp, target_t);
      exit(EXIT_FAILURE);
   }
   *new_timestamp = target_t;

   // Get destination coordinate.
   const int current_x = x - std::stoi(s_dx);
   const int current_y = y - std::stoi(s_dy);
   const Snapshot &s_current = (*timeline)[timeline->size() - 2];
   Snapshot &s_target = (*timeline)[target_t];
   const int target_x = current_x - (s_current.x - s_target.x);
   const int target_y = current_y - (s_current.y - s_target.y);
   if( target_x < 0 ||
       target_y < 0 ||
       target_y >= static_cast<int>(s_target.grid.size()) ||
       target_x >= static_cast<int>(s_target.grid.front().size()) )
   {
      // XXX Automatic resizing not implemented yet.
      printf("Out of bounds write: t=%d, x=%d, y=%d\n",
             target_t, target_x, target_y);
      exit(EXIT_FAILURE);
   }

   Cell &target_cell = s_target.grid[target_y][target_x];
   if( !target_cell.empty() && target_cell[0] == 'S' )
   {
      target_cell = v;
      *output = v;
      return true;
   }
   target_cell = v;
   return false;
}

// Run a single simulation step, returns true if simulation is done.
static bool Step(Timeline *timeline, int *timestamp, std::string *output)
{
   if( ResizeTimeline(timeline, timestamp) )
      return Step(timeline, timestamp, output);

   const Snapshot &s0 = (*timeline)[*timestamp];
   const Grid &t0 = s0.grid;
   const int height = static_cast<int>(t0.size());
   const int width = static_cast<int>(t0.front().size());

   ++*timestamp;
   if( *timestamp >= static_cast<int>(timeline->size()) )
   {
      // Append new snapshot.
      timeline->push_back(s0);
   }
   else
   {
      // Overwrite existing snapshot (after travelling back in time).
      (*timeline)[*timestamp] = s0;
   }

   Grid &t1 = (*timeline)[*timestamp].grid;
   PendingWrites pending_writes;
   bool got_output = false;
   int new_timestamp = -1;
   for(int y = 0; y < height; y++)
   {
      for(int x = 0; x < width; x++)
      {
         if( t0[y][x].size() != 1 )
            continue;
         switch( t0[y][x][0] )
         {
            case '<':
               if( !t0[y][x + 1].empty() )
               {
                  if( Write(t1, x - 1, y, t0[y][x + 1],
                            &pending_writes, output) )
                  {
                     got_output = true;
                  }
                  pending_writes.remove.push_back(std::make_pair(x + 1, y));
               }
               break;

            case '>':
               if( !t0[y][x - 1].empty() )
               {
                  if( Write(t1, x + 1, y, t0[y][x - 1],
                            &pending_writes, output) )
                  {
                     got_output = true;
                  }
                  pending_writes.remove.push_back(std::make_pair(x - 1, y));
               }
               break;

            case '^':
               if( !t0[y + 1][x].empty() )
               {
                  if( Write(t1, x, y - 1, t0[y + 1][x],
                            &pending_writes, output) )
                  {
                     got_output = true;
                  }
                  pending_writes.remove.push_back(std::make_pair(x, y + 1));
               }
               break;

            case 'v':
               if( !t0[y - 1][x].empty() )
               {
                  if( Write(t1, x, y + 1, t0[y - 1][x],
                            &pending_writes, output) )
                  {
                     got_output = true;
                  }
                  pending_writes.remove.push_back(std::make_pair(x, y - 1));
               }
               break;

            case '+':
            case '-':
            case '*':
            case '/':
            case '%':
               if( Arith(t0, t1, x, y, &pending_writes, output) )
                  got_output = true;
               break;

            case '=':
            case '#':
               if( Compare(t0, t1, x, y, &pending_writes, output) )
                  got_output = true;
               break;

            case '@':
               if( TimeWarp(timeline, timestamp, t0, x, y,
                            &new_timestamp, output) )
               {
                  got_output = true;
               }
               break;

            default:
               // Silently ignore all other operators, including 'S'.
               break;
         }
      }
   }
   if( new_timestamp >= 0 )
      *timestamp = new_timestamp;

   // Apply removals.
   for(const auto &[x, y] : pending_writes.remove)
   {
      if( pending_writes.add.contains(std::make_pair(x, y)) )
         continue;
      t1[y][x].clear();
   }

   return got_output;
}

}  // namespace

int main(int argc, char **argv)
{
   if( argc < 2 )
      return printf("%s {input.txt} {a} {b}\n", *argv);

   Timeline timeline;
   timeline.reserve(kMaxTicks);

   timeline.push_back(Snapshot());
   timeline.front().grid = Load(argv[1]);
   const char *output_warning =
      HasOutput(timeline.front().grid) ? "" : " (missing output)";

   if( argc >= 3 )
   {
      XYList input_positions;
      AddInput(timeline.front().grid, 'A', argv[2], &input_positions);
      if( argc >= 4 )
         AddInput(timeline.front().grid, 'B', argv[3], &input_positions);
      puts("Input positions");
      Dump(timeline.front().grid, input_positions);
   }

   int timestamp = 0;
   int previous_timetamp = 0;
   std::string output;
   for(int tick = 1; tick < kMaxTicks; tick++)
   {
      printf("tick=%d, t=%d%s\n", tick, timestamp + 1, output_warning);
      DumpWithDiff(timeline[previous_timetamp], timeline[timestamp]);

      previous_timetamp = timestamp;
      if( Step(&timeline, &timestamp, &output) )
         break;
   }

   printf("t=%d%s\n", timestamp + 1, output_warning);
   DumpWithDiff(timeline[previous_timetamp], timeline[timestamp]);
   printf("output=%s\n", output.c_str());
   return 0;
}
