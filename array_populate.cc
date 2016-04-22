#include <cstdio>
#include <cstdlib>

#include "legion.h"

#define ROW 5
#define COL 3

using namespace LegionRuntime::HighLevel;
using namespace LegionRuntime::Accessor;

enum TASK_ID  {
  TOP_LEVEL_TASK_ID,
  PRINT_LR_TASK_ID
};

void top_level_task(const Task *task, const std::vector<PhysicalRegion> &regions, Context ctx, HighLevelRuntime *runtime)
{
  int field_id[COL];

  Rect<1> elem_rect(Point<1>(0), Point<1>(ROW - 1));
  IndexSpace is = runtime->create_index_space(ctx, Domain::from_rect<1>(elem_rect));
  FieldSpace fs = runtime->create_field_space(ctx);
  {
    FieldAllocator allocator = runtime->create_field_allocator(ctx, fs);

    for(int i = 0; i < COL; i++)  {
      field_id[i] = allocator.allocate_field(sizeof(int));
    }
  }

  LogicalRegion input_lr = runtime->create_logical_region(ctx, is, fs);

  RegionRequirement req(input_lr, READ_WRITE, EXCLUSIVE, input_lr);

  /* specify which fields on logical regions to request */
  for(int i = 0; i < COL; i++)  {
    req.add_field(field_id[i]);
  }

  // req.add_field(field_id[0]);
  // req.add_field(field_id[1]);
  // req.add_field(field_id[2]);
  // req.add_field(field_id[3]);
  // req.add_field(field_id[4]);

  InlineLauncher input_launcher(req);
  PhysicalRegion input_region = runtime->map_region(ctx, input_launcher);
  input_region.wait_until_valid();

  RegionAccessor<AccessorType::Generic, int> region_accessor[COL];

  for(int i = 0; i < COL; i++)  {
    region_accessor[i] = input_region.get_field_accessor(field_id[i]).typeify<int>();
  }

  for(GenericPointInRectIterator<1> pir(elem_rect); pir; pir++) {
    for(int i = 0; i < COL; i++)  {
       region_accessor[i].write(DomainPoint::from_point<1>(pir.p), (rand() % 100));
    }
  }

  TaskLauncher print_lr_launcher(PRINT_LR_TASK_ID, TaskArgument(field_id, sizeof(field_id)));
  print_lr_launcher.add_region_requirement(RegionRequirement(input_lr, READ_ONLY, EXCLUSIVE, input_lr));
  for(int i = 0; i < COL; i++)  {
    print_lr_launcher.add_field(0, field_id[i]);
  }
  runtime->execute_task(ctx, print_lr_launcher);


  /* GenericPointInRectIterator iterates through a row (index-space) */

  // for(GenericPointInRectIterator<1> pir(elem_rect); pir; pir++) {
  //   for(int i = 0; i < COL; i++)  {
  //     int x = region_accessor[i].read(DomainPoint::from_point<1>(pir.p));
  //     printf(" %d", x);
  //   }
  //   printf("\n");
  // }
  printf("\n Done!\n");

}

void print_lr_task(const Task *task,
            const std::vector<PhysicalRegion> &regions,
            Context ctx, HighLevelRuntime *runtime) {

  int *field_id = (int *) task->args;

  RegionAccessor<AccessorType::Generic, int> region_accessor[COL];
  for(int i = 0; i < COL; i++)  {
    region_accessor[i] = regions[0].get_field_accessor(field_id[i]).typeify<int>();
  }

  Domain domain = runtime->get_index_space_domain(ctx, task->regions[0].region.get_index_space());
  Rect <1> rect = domain.get_rect<1>();

  printf("\n Printing Loaded Values:\n");

  for(GenericPointInRectIterator<1> pir(rect); pir; pir++) {
    for(int i = 0; i < COL; i++)  {
      int x = region_accessor[i].read(DomainPoint::from_point<1>(pir.p));
      printf("  > %d", x);
    }
    printf("\n");
  }

}



int main(int argc, char **argv) {
  HighLevelRuntime::set_top_level_task_id(TOP_LEVEL_TASK_ID);

  HighLevelRuntime::register_legion_task<top_level_task>(TOP_LEVEL_TASK_ID, Processor::LOC_PROC, true, false);
  HighLevelRuntime::register_legion_task<print_lr_task>(PRINT_LR_TASK_ID, Processor::LOC_PROC, true, false);


  return HighLevelRuntime::start(argc, argv);
}
