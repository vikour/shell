
#include <defs.h>
#include <jobs_control.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

/// INIT_JOB

/// CREATE JOB -----------------------------------------------

// - Caja negra
// list job = NULL, cmd valid
void t_create_job_1() {
    printf("Testing 1 ...");
    assert(create_job(NULL, "ls") == NULL);
    printf("OK!\n");
}

// list_job valid, cmd null
void t_create_job_2() {
    ListJobs lj = NULL;
    printf("Testing 2 ...");
    assert(create_job(&lj,NULL) == NULL);
    printf("OK!\n");
}

// list job isn't empty after job created.
void t_create_job_3() {
    ListJobs lj = NULL;
    printf("Testing 3 ...");
    create_job(&lj,"ls");
    assert(lj != NULL);
    printf("OK!\n");
}

// list_job valid, cmd ""
void t_create_job_4() {
    ListJobs lj = NULL;
    printf("Testing 4 ...");
    assert(create_job(&lj, "")->proc->args[0] == '\0');
    printf("OK!\n");
}

// list_job valid, cmd "c 1 2 3 4"
void t_create_job_5() {
    ListJobs lj = NULL;
    Job * job;
    printf("Testing 5 ...");
    job = create_job(&lj,"c 1 2 3 4");
    assert(strcmp(job->proc->args[0], "c") == 0);
    assert(strcmp(job->proc->args[1], "1") == 0);
    assert(strcmp(job->proc->args[2], "2") == 0);
    assert(strcmp(job->proc->args[3], "3") == 0);
    assert(strcmp(job->proc->args[4], "4") == 0);
    assert(job->proc->args[5] == NULL);
    assert(job->foreground);
    printf("OK!\n");
}

// list_job valid, cmd "c '1 2 3 4'"
void t_create_job_6() {
    ListJobs lj = NULL;
    Job * job;
    printf("Testing 6 ...");
    job = create_job(&lj,"c '1 2 3 4'");
    assert(strcmp(job->proc->args[0], "c") == 0);
    assert(strcmp(job->proc->args[1], "'1 2 3 4'") == 0);
    assert(job->proc->args[2] == NULL);
    assert(job->foreground);
    printf("OK!\n");
}

// list_job_valid, cmd "c "1 2 3 4" "
void t_create_job_7() {
    ListJobs lj = NULL;
    Job * job;
    printf("Testing 7 ...");
    job = create_job(&lj,"c \"1 2 3 4\"");
    assert(strcmp(job->proc->args[0], "c") == 0);
    assert(strcmp(job->proc->args[1], "\"1 2 3 4\"") == 0);
    assert(job->proc->args[2] == NULL);
    assert(job->foreground);
    printf("OK!\n");
    
}

// list_job_valid, cmd "c "1 2 '3 4'" "
void t_create_job_8() {
    ListJobs lj = NULL;
    Job * job;
    printf("Testing 8 ...");
    job = create_job(&lj,"c \"1 2 '3 4'\"");
    assert(strcmp(job->proc->args[0], "c") == 0);
    assert(strcmp(job->proc->args[1], "\"1 2 '3 4'\"") == 0);
    assert(job->proc->args[2] == NULL);
    assert(job->foreground);
    printf("OK!\n");
}

void t_create_job_9() {
    ListJobs lj = NULL;
    Job * job;
    printf("Testing 9 ...");
    job = create_job(&lj,"c '1 2' '3 4'");
    assert(strcmp(job->proc->args[0], "c") == 0);
    assert(strcmp(job->proc->args[1], "'1 2'") == 0);
    assert(strcmp(job->proc->args[2], "'3 4'") == 0);
    assert(job->proc->args[3] == NULL);
    assert(job->foreground);
    printf("OK!\n");
}

// If there has a pipe in command, there must have another
// process next to first.
void t_create_job_10() {
    ListJobs lj = NULL;
    Job * job;
    printf("Testing 10 ...");
    job = create_job(&lj,"c 1 | c2");
    assert(job->proc->next != NULL);
    assert(job->foreground);
    printf("OK!\n");
}

// list_job_valid, cmd "c 1 2 3 | c2 1 2 3 4"
void t_create_job_11() {
    ListJobs lj = NULL;
    Job * job;
    printf("Testing 11 ...");
    job = create_job(&lj,"c 1 2 3 | c2 1 2 3 4");
    assert(strcmp(job->proc->args[0], "c") == 0);
    assert(strcmp(job->proc->args[1], "1") == 0);
    assert(strcmp(job->proc->args[2], "2") == 0);
    assert(strcmp(job->proc->args[3], "3") == 0);
    assert(job->proc->args[4] == NULL);
    assert(strcmp(job->proc->next->args[0], "c2") == 0);
    assert(strcmp(job->proc->next->args[1], "1") == 0);
    assert(strcmp(job->proc->next->args[2], "2") == 0);
    assert(strcmp(job->proc->next->args[3], "3") == 0);
    assert(strcmp(job->proc->next->args[4], "4") == 0);
    assert(job->proc->next->args[5] == NULL);
    assert(job->foreground);
    printf("OK!\n");
}
// testing spaces between args and before them.
void t_create_job_12() {
    ListJobs lj = NULL;
    Job * job;
    printf("Testing 12 ...");
    job = create_job(&lj, "    c 1     2     3  4       ");
    assert(strcmp(job->proc->args[0], "c") == 0);
    assert(strcmp(job->proc->args[1], "1") == 0);
    assert(strcmp(job->proc->args[2], "2") == 0);
    assert(strcmp(job->proc->args[3], "3") == 0);
    assert(strcmp(job->proc->args[4], "4") == 0);
    assert(job->proc->args[5] == NULL);
    assert(job->foreground);
    printf("OK!\n");
}

//        (testing background projects)
void t_create_job_13() {
    ListJobs lj = NULL;
    Job * job;
    printf("Testing 13 ...");
    job = create_job(&lj, "c &");
    assert(!job->foreground);
    printf("OK!\n");
}

void t_create_job_14() {
    ListJobs lj = NULL;
    Job * job;
    printf("Testing 14 ...");
    job = create_job(&lj, "c &       ");
    assert(!job->foreground);
    printf("OK!\n");
}

void t_create_job_15() {
    ListJobs lj = NULL;
    Job * job;
    printf("Testing 15 ...");
    job = create_job(&lj, "c & c"); // Trunca el comando.
    assert(!job->foreground);
    printf("OK!\n");
}

void t_create_job_16() {
    ListJobs lj = NULL;
    Job * job;
    printf("Testing 16 ...");
    job = create_job(&lj, "c & c &");
    assert(!job->foreground);
    printf("OK!\n");
}

// test max args.
void t_create_job_17() {
    int i = 0;
    ListJobs lj = NULL;
    Job * job;
    char cmd [MAX_LINE_COMMAND];
    char * ptr = cmd;
    
    printf("Testing 17 ...");
    
    for (i = 0 ; i < MAX_ARGS ; i++)  {
        *ptr = '0' + (i % 10);
        *(ptr + 1) = ' ';
        ptr +=2;
    }
    
    *(ptr - 1) = '\0';
    job = create_job(&lj,cmd);
    assert(job->proc->args[MAX_ARGS-1] != NULL);
    assert(job->proc->args[MAX_ARGS] == NULL);
    printf("...OK!\n");
}
// args exceed of range.
void t_create_job_18() {
    int i = 0;
    ListJobs lj = NULL;
    Job * job;
    char cmd [MAX_LINE_COMMAND];
    char * ptr = cmd;
    
    printf("Testing 18 ...");
    
    for (i = 0 ; i < MAX_ARGS + 1 ; i++)  {
        *ptr = '0' + (i % 10);
        *(ptr + 1) = ' ';
        ptr +=2;
    }
    
    *(ptr - 1) = '\0';
    job = create_job(&lj,cmd);
    assert(job->proc->args[MAX_ARGS-1] != NULL);
    assert(job->proc->args[MAX_ARGS] == NULL);
    printf("...OK!\n");
}

void t_create_job() {
    printf("\nTesting create_job ...\n");
    t_create_job_1();
    t_create_job_2();
    t_create_job_3();
    t_create_job_4();
    t_create_job_5();
    t_create_job_6();
    t_create_job_7();
    t_create_job_8();
    t_create_job_9();
    t_create_job_10();
    t_create_job_11();
    t_create_job_12();
    t_create_job_13();
    t_create_job_14();
    t_create_job_15();
    t_create_job_16();
    t_create_job_17();
    t_create_job_18();
    printf("..... All right!\n");
    
}


// CREATE_JOB && REMOVE_JOB
void t_linked_list_job_1() {
    ListJobs jl = NULL;
    Job * a, *b;
    
    a = create_job(&jl,"a");
    b = create_job(&jl,"b");
    
    printf("Testing 1 ...");
    assert(b == a->next);
    assert(b->next == NULL);
    printf("OK!\n");
}

void t_linked_list_job_2() {
    ListJobs jl = NULL;
    Job *a;
    
    a =create_job(&jl,"a");
    a->gpid = 1;
    remove_job(&jl, 1);
    
    printf("Testing 2 ...");
    assert(jl == NULL);
    printf("OK!\n");
}

void t_linked_list_job_3() {
    ListJobs jl = NULL;
    Job * a, *b;
    
    a = create_job(&jl,"a");
    b = create_job(&jl,"b");
    a->gpid = 1;
    b->gpid = 2;
    remove_job(&jl, 1);
    
    printf("Testing 3 ...");
    assert(jl == b);
    assert(b->next == NULL);
    printf("OK!\n");
}

void t_linked_list_job_4() {
    ListJobs jl = NULL;
    Job * a, *b;
    
    a = create_job(&jl,"a");
    a->gpid = 0;
    b = create_job(&jl,"b");
    b->gpid = 1;
    remove_job(&jl, b->gpid);
    
    printf("Testing 4 ...");
    assert(jl == a);
    assert(a->next == NULL);
    printf("OK!\n");
}

void t_linked_list_job_5() {
    ListJobs jl = NULL;
    Job * a, *b, *c;
    
    a = create_job(&jl,"a");
    a->gpid = 1;
    b = create_job(&jl,"b");
    b->gpid = 2;
    c = create_job(&jl,"c");
    c->gpid = 3;
    remove_job(&jl, 2);
    
    printf("Testing 5 ...");
    assert(jl == a);
    assert(a->next == c);
    assert(c->next == NULL);
    printf("OK!\n");
}

void t_linked_list_job() {
    printf("\nTesting linked_list_job....\n");
    t_linked_list_job_1();
    t_linked_list_job_2();
    t_linked_list_job_3();
    t_linked_list_job_4();
    t_linked_list_job_5();
    printf("... All Right!\n");
}

void t_analyce_next_status_1() {
    Job job;
    
    job.status = READY;
    job.foreground = 1;
    
    printf("Testing 1 ...");
    analize_next_state(&job, 0, 0, 1);
    assert(job.status == RUNNING );
    assert(job.foreground);
    printf("OK!\n");
}

void t_analyce_next_status_2() {
    Job job;
    
    job.status = job_ready;
    job.foreground = 0;
    
    printf("Testing 2 ...");
    analize_next_state(&job, 0, 0, 1);
    assert(job.status == job_executed );
    assert(!job.foreground);
    printf("OK!\n");
}

void t_analyce_next_status_3() {
    Job job;
    
    job.status = job_ready;
    job.foreground = 1;
    
    printf("Testing 3 ...");
    analize_next_state(&job, 0, 0, 0);
    assert(job.status == job_ready );
    printf("OK!\n");
}

void t_analyce_next_status_4() {
    Job job;
    
    job.status = job_ready;
    job.foreground = 0;
    
    printf("Testing 4 ...");
    analize_next_state(&job, 0, 0, 0);
    assert(job.status == job_ready );
    printf("OK!\n");
}

void t_analyce_next_status() {
    printf("\nTesting t_analyce_next_status....\n");
    t_analyce_next_status_1();
    t_analyce_next_status_2();
    t_analyce_next_status_3();
    t_analyce_next_status_4();
    printf("... All Right!\n");
}

int main() {
    
    t_create_job();
    t_linked_list_job();
    t_analyce_next_status();
    
    return 0;
}
