#include "hazard_pointers.h"

namespace Reactive {

thread_local Hazard_Pointers::Thread_Record Hazard_Pointers::d_thread_record;
atomic<int> Hazard_Pointers::d_pointer_alloc_hint;
array<Hazard_Pointer<void>,Hazard_Pointers::H> Hazard_Pointers::d_pointers;


}
