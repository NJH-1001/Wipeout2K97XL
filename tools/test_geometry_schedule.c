#include "geometry_schedule.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
    for(unsigned steps=2;steps<=4;++steps) {
        unsigned seen=0,previous=0;
        for(unsigned refresh=0;refresh<2;++refresh) {
            unsigned first,end;
            assert(geometry_schedule_window(steps,refresh,&first,&end));
            assert(first==previous && end>first);
            for(unsigned k=first;k<end;++k) {
                /* Every presentation deadline falls in its original refresh
                 * interval; each geometric phase is emitted exactly once. */
                assert(k*2>=refresh*steps && k*2<(refresh+1)*steps);
                assert(!(seen&(1u<<k)));seen|=1u<<k;
            }
            previous=end;
        }
        assert(previous==steps && seen==((1u<<steps)-1));
        unsigned first,end;
        assert(!geometry_schedule_window(steps,2,&first,&end));
    }
    unsigned first,end;
    assert(!geometry_schedule_window(1,0,&first,&end));
    assert(!geometry_schedule_window(5,0,&first,&end));
    puts("PASS: 60/90/120 schedules preserve both guest refresh intervals and emit every phase once");
}
