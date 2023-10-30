// test.cpp
// Derek Chiou
//     Oct. 8, 2023


#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "generic_error.h"
#include "helpers.h"
#include "cache.h"
#include "test.h"

proc_t::proc_t(int __p) {
    proc = __p;
    test_step = 0;
    init();
}

void proc_t::init() {
    response.retry_p = false;
    ld_p = false;
}

void proc_t::bind(cache_t *c) {
    cache = c;
}


// ***** FYTD ***** 

// this is just a simple random test. I'm not giving
// you any more test cases than this. You will be tested on the
// correctness and performance of your solution.

extern args_t args;
extern int addr_range;
extern cache_t **caches;

test_args_t test_args;
int livelock_test;

void init_test() {
    switch(args.test) {
    case 0:
        test_args.addr_range = args.num_procs*512;
        test_args.stage = 0;
        break;
    case 1:
        if(args.num_procs != 2)
            ERROR("this test requires 2 cpus");
        test_args.addr_range = args.num_procs*512;
        test_args.stage = 0;
        test_args.data = new int[5];
        break;
    case 2:
        if(args.num_procs < 2)
            ERROR("this test requires at least 2 cpus");
        test_args.addr_range = args.num_procs*512;
        test_args.stage = 0;
        test_args.data = new int[10];
        break;

    default:
        ERROR("don't recognize this test");
    }
}

void finish_test() {
    double hr;

    for (int i = 0; i < args.num_procs; ++i) {
        switch(args.test) {
        case 0:
            hr = caches[i]->hit_rate();
            if (!within_tolerance(hr, 0.5, 0.01)) {
                ERROR("out of tolerance");
            }
            break;
        case 1:
        case 2:
            delete[] test_args.data;
            break;
            
        default: 
            ERROR("don't recognize this test");
        }
    }
    printf("passed\n");
}

void proc_t::advance_one_cycle() 
{
    switch (args.test) 
    {
    case 0:
        if (!response.retry_p) 
        {
            addr = random() % test_args.addr_range;
            ld_p = ((random() % 2) == 0);
        }
        if (ld_p) response = cache->load(addr, 0, &data_load, response.retry_p);
        else      response = cache->store(addr, 0, cur_cycle, response.retry_p);
        break;
    case 1:
        if (!response.retry_p) 
        {
            if(proc == 0)
            {
                switch(test_step)
                {
                case 0: //read addr 1: I->E
                    addr = 1;
                    ld_p = true;
                    test_step++;
                    break;
                case 1: //write addr 1: E->M
                    addr = 1;
                    ld_p = false;
                    data_store = random();
                    test_args.data[0] = data_store;
                    test_step++;
                    break;
                case 2: //read addr 20: I->S. read rq addr 1: M->S
                    test_args.stage = 1;
                    addr = 20;
                    ld_p = true;
                    test_step++;
                    break;
                case 3: //stall. write rq addr 1: S->I 
                    addr = 0;
                    if(test_args.stage == 2) 
                        test_step++;
                    break;
                case 4: // write addr 1: I->M
                    addr = 1;
                    ld_p = false;
                    data_store = random();
                    test_args.data[2] = data_store;
                    test_step++;
                    break;
                default:
                    addr = 0;
                    break;
                }
            } 
            else if (proc == 1)
            {
                switch(test_step)
                {
                case 0: //read addr 20: I->E
                    addr = 20;
                    ld_p = true;
                    test_step++;
                    break;
                case 1: //stall
                    addr = 0;
                    if(test_args.stage == 1) test_step++;
                    break;
                case 2: //read addr 1: I->S. read rq addr 20: E->S
                    addr = 1;
                    ld_p = true;
                    test_step++;
                    break;
                case 3: // write addr 1: S->M
                    if(data_load != test_args.data[0])
                        ERROR("incorrect data loaded");
                    addr = 1;
                    ld_p = false;
                    data_store = random();
                    test_args.data[1] = data_store;
                    test_step++;
                    break;
                case 4: // stall. write rq addr 1: M->I
                    test_args.stage = 2;
                    addr = 0;
                    test_step++;
                    break;
                default:
                    addr = 0;
                    break;
                }
            }
        }
        if(addr && ld_p) 
            response = cache->load(addr, 0, &data_load, response.retry_p);
        else if(addr)
            response = cache->store(addr, 0, data_store, response.retry_p);
        break;
    case 2:
        if (!response.retry_p) 
        {

            switch(test_step)
            {
            case 0: //everyone reads addr 1
                if(proc == 0) // reset
                    for(int i = 0; i<10; i++)
                        test_args.data[i] = 0;
                addr = 1;
                ld_p = true;
                test_step++;
                break;
            case 1: //wait and record completion. check data
                addr = 0;
                if(test_args.data[0] == 0)
                    test_args.data[1] = data_load;
                else if(test_args.data[1] != data_load)
                    ERROR("data read does not match.");
                test_args.data[0]++;
                test_step++;
                break;
            case 2: //check everyone complete before continueing
                addr = 0;
                if(test_args.data[0] == args.num_procs) 
                    test_step++;
                break;
            case 3: // everyone writes addr 1
                addr = 1;
                ld_p = false;
                data_store = proc;
                test_step++;
                break;
            case 4: // wait and record completion
                addr = 0;
                test_args.data[2]++;
                test_step++;
                break;
            case 5: //check everyone complete before continueing
                addr = 0;
                if(test_args.data[2] == args.num_procs) 
                    test_step++;
                break;
            case 6: //everyone reads addr 1
                addr = 1;
                ld_p = true;
                test_step++;
                break;
            case 7: //stall and record completion. check data
                addr = 0;
                if(test_args.data[3] == 0)
                    test_args.data[4] = data_load;
                else if(test_args.data[4] != data_load)
                    ERROR("data read does not match.");
                test_args.data[3]++;
                test_step++;
                break;
            case 8: //check everyone complete before continueing
                addr = 0;
                if(test_args.data[3] == args.num_procs) 
                    test_step++;
                break;
            case 9: // everyone writes addr 20
                addr = 20;
                ld_p = false;
                data_store = proc;
                test_step++;
                break;
            case 10: // wait and record completion
                addr = 0;
                test_args.data[5]++;
                test_step++;
                break;
            case 11: //check everyone complete before continueing
                addr = 0;
                if(test_args.data[5] == args.num_procs) 
                    test_step++;
                break;
            case 12: //everyone reads addr 20
                addr = 20;
                ld_p = true;
                test_step++;
                break;
            case 13: //stall and record completion. check data
                addr = 0;
                if(test_args.data[6] == 0)
                    test_args.data[7] = data_load;
                else if(test_args.data[7] != data_load)
                    ERROR("data read does not match.");
                test_args.data[6]++;
                test_step++;
                break;
            case 14: //check everyone complete before continueing
                addr = 0;
                if(test_args.data[6] == args.num_procs) 
                    test_step++;
                break;
            default:
                addr = 0;
                break;
            }
        }
        if(addr && ld_p) 
            response = cache->load(addr, 0, &data_load, response.retry_p);
        else if(addr)
            response = cache->store(addr, 0, data_store, response.retry_p);
        break;
    default:
        ERROR("don't know this test case");
    }
}
