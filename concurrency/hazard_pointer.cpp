#include "hazard_pointers.h"

namespace Concurrent {

thread_local Hazard_Pointers::Thread_Record Hazard_Pointers::d_thread_record;
array<Hazard_Pointers::Private_Record, Hazard_Pointers::H> Hazard_Pointers::d_records;

}
