#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_JOBS   500
#define MAX_LINE   512
#define DATA_FILE  "../data/jobs.txt"
#define FIELD_SIZE 128

typedef struct {
    int  id;
    char reg_no[FIELD_SIZE];
    char owner_name[FIELD_SIZE];
    char phone[FIELD_SIZE];
    char engine_no[FIELD_SIZE];
    char service_type[FIELD_SIZE];
    char delivery_date[FIELD_SIZE];
    char status[FIELD_SIZE];
    int  priority;
    char extra[FIELD_SIZE];
} JobCard;

#define MAX_TREE_NODES 500000

typedef struct STNode {
    int first_child; 
    int next_sibling;
    char edge_char;
    int *job_ids;
    int job_count;
    int job_cap;
} STNode;

extern STNode st_nodes[MAX_TREE_NODES];
extern int st_node_count;
STNode st_nodes[MAX_TREE_NODES];
int st_node_count = 1;

int st_new_node(char c) {
    if (st_node_count >= MAX_TREE_NODES) return 0;
    int idx = st_node_count++;
    memset(&st_nodes[idx], 0, sizeof(STNode));
    st_nodes[idx].edge_char = c;
    return idx;
}

void st_add_job(int node_idx, int job_id) {
    if (node_idx == 0) return;
    STNode *n = &st_nodes[node_idx];
    for (int i = 0; i < n->job_count; i++) {
        if (n->job_ids[i] == job_id) return;
    }
    if (n->job_count >= n->job_cap) {
        n->job_cap = n->job_cap == 0 ? 4 : n->job_cap * 2;
        n->job_ids = (int*)realloc(n->job_ids, n->job_cap * sizeof(int));
    }
    n->job_ids[n->job_count++] = job_id;
}

int st_find_or_create_child(int node_idx, char c) {
    int child = st_nodes[node_idx].first_child;
    int prev = -1;
    while(child != 0) {
        if(st_nodes[child].edge_char == c) return child;
        prev = child;
        child = st_nodes[child].next_sibling;
    }
    int new_c = st_new_node(c);
    if(prev == -1) {
        st_nodes[node_idx].first_child = new_c;
    } else {
        st_nodes[prev].next_sibling = new_c;
    }
    return new_c;
}

void st_insert_suffix(const char *suffix, int job_id) {
    int curr = 0; 
    for (int i = 0; suffix[i] != '\0'; i++) {
        unsigned char c = (unsigned char)suffix[i];
        if (c >= 'A' && c <= 'Z') c += 32;
        curr = st_find_or_create_child(curr, c);
        if (curr == 0) break;
        st_add_job(curr, job_id);
    }
}

void st_insert_string(const char *str, int job_id) {
    if (!str) return;
    int len = strlen(str);
    for (int i = 0; i < len; i++) {
        st_insert_suffix(str + i, job_id);
    }
}

void st_index_job(JobCard *j) {
    st_insert_string(j->reg_no, j->id);
    st_insert_string(j->owner_name, j->id);
    st_insert_string(j->phone, j->id);
    st_insert_string(j->engine_no, j->id);
}

typedef struct MinHeap {
    JobCard **jobs;
    int size;
    int capacity;
} MinHeap;

void heap_swap(JobCard **a, JobCard **b) {
    JobCard *t = *a;
    *a = *b;
    *b = t;
}

void heapify_up(MinHeap *heap, int idx) {
    if (idx && heap->jobs[(idx - 1) / 2]->priority > heap->jobs[idx]->priority) {
        heap_swap(&heap->jobs[idx], &heap->jobs[(idx - 1) / 2]);
        heapify_up(heap, (idx - 1) / 2);
    }
}

void heapify_down(MinHeap *heap, int idx) {
    int smallest = idx;
    int left = 2 * idx + 1;
    int right = 2 * idx + 2;

    if (left < heap->size && heap->jobs[left]->priority < heap->jobs[smallest]->priority)
        smallest = left;
    if (right < heap->size && heap->jobs[right]->priority < heap->jobs[smallest]->priority)
        smallest = right;

    if (smallest != idx) {
        heap_swap(&heap->jobs[idx], &heap->jobs[smallest]);
        heapify_down(heap, smallest);
    }
}

void heap_insert(MinHeap *heap, JobCard *job) {
    if (heap->size == heap->capacity) return;
    heap->jobs[heap->size] = job;
    heapify_up(heap, heap->size);
    heap->size++;
}

JobCard* heap_extract_min(MinHeap *heap) {
    if (heap->size == 0) return NULL;
    JobCard *root = heap->jobs[0];
    heap->jobs[0] = heap->jobs[heap->size - 1];
    heap->size--;
    heapify_down(heap, 0);
    return root;
}

static void trim_newline(char *s) {
    size_t len = strlen(s);
    while (len > 0 && (s[len-1] == '\n' || s[len-1] == '\r')) {
        s[--len] = '\0';
    }
}

static void safe_copy(char *dest, const char *src, int dest_size) {
    int i = 0;
    while (i < dest_size - 1 && src[i] != '\0') {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

static int parse_line(const char *line, JobCard *j) {
    char buf[MAX_LINE];
    strncpy(buf, line, MAX_LINE - 1);
    buf[MAX_LINE - 1] = '\0';
    trim_newline(buf);

    char *fields[10];
    int   nf = 0;
    char *p  = buf;

    fields[nf++] = p;
    while (*p && nf < 10) {
        if (*p == '|') {
            *p = '\0';
            fields[nf++] = p + 1;
        }
        p++;
    }

    if (nf < 10) return 0; 

    j->id = atoi(fields[0]);
    safe_copy(j->reg_no,        fields[1], FIELD_SIZE);
    safe_copy(j->owner_name,    fields[2], FIELD_SIZE);
    safe_copy(j->phone,         fields[3], FIELD_SIZE);
    safe_copy(j->engine_no,     fields[4], FIELD_SIZE);
    safe_copy(j->service_type,  fields[5], FIELD_SIZE);
    safe_copy(j->delivery_date, fields[6], FIELD_SIZE);
    safe_copy(j->status,        fields[7], FIELD_SIZE);
    j->priority = atoi(fields[8]);
    safe_copy(j->extra,         fields[9], FIELD_SIZE);

    return 1;
}

int fh_read_all(JobCard *jobs, int max) {
    FILE *fp = fopen(DATA_FILE, "r");
    if (!fp) return 0;

    int count = 0;
    char line[MAX_LINE];
    while (count < max && fgets(line, sizeof(line), fp)) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;
        if (parse_line(line, &jobs[count])) count++;
    }
    fclose(fp);
    return count;
}

int fh_write_all(JobCard *jobs, int count) {
    FILE *fp = fopen(DATA_FILE, "w");
    if (!fp) return 0;

    fprintf(fp, "# Two-Wheeler Service Jobs\n");
    fprintf(fp, "# ID|RegNo|OwnerName|Phone|EngineNo|ServiceType|DeliveryDate|Status|Priority|Extra\n");
    for (int i = 0; i < count; i++) {
        fprintf(fp, "%d|%s|%s|%s|%s|%s|%s|%s|%d|%s\n",
            jobs[i].id,
            jobs[i].reg_no,
            jobs[i].owner_name,
            jobs[i].phone,
            jobs[i].engine_no,
            jobs[i].service_type,
            jobs[i].delivery_date,
            jobs[i].status,
            jobs[i].priority,
            jobs[i].extra);
    }
    fclose(fp);
    return 1;
}

int fh_append(JobCard *j) {
    FILE *fp = fopen(DATA_FILE, "r");
    int has_header = 0;
    if (fp) {
        char line[MAX_LINE];
        if (fgets(line, sizeof(line), fp)) has_header = (line[0] == '#');
        fclose(fp);
    }

    fp = fopen(DATA_FILE, "a");
    if (!fp) return 0;

    if (!has_header) {
        fprintf(fp, "# Two-Wheeler Service Jobs\n");
        fprintf(fp, "# ID|RegNo|OwnerName|Phone|EngineNo|ServiceType|DeliveryDate|Status|Priority|Extra\n");
    }

    fprintf(fp, "%d|%s|%s|%s|%s|%s|%s|%s|%d|%s\n",
        j->id, j->reg_no, j->owner_name, j->phone,
        j->engine_no, j->service_type, j->delivery_date,
        j->status, j->priority, j->extra);
    fclose(fp);
    return 1;
}

int fh_next_id(void) {
    JobCard jobs[MAX_JOBS];
    int count = fh_read_all(jobs, MAX_JOBS);
    int max_id = 0;
    for (int i = 0; i < count; i++) {
        if (jobs[i].id > max_id) max_id = jobs[i].id;
    }
    return max_id + 1;
}

int calc_priority(const char *delivery_date) {
    struct tm due = {0};
    if (sscanf(delivery_date, "%d-%d-%d",
               &due.tm_year, &due.tm_mon, &due.tm_mday) != 3) return 0;
    due.tm_year -= 1900;
    due.tm_mon  -= 1;
    due.tm_isdst = -1;

    time_t now   = time(NULL);
    time_t due_t = mktime(&due);
    double diff  = difftime(due_t, now) / 86400.0;
    return (int)diff;  
}

int jc_add(const char *reg_no, const char *owner_name, const char *phone,
           const char *engine_no, const char *service_type,
           const char *delivery_date, const char *extra) {
    JobCard j;
    memset(&j, 0, sizeof(j));

    j.id = fh_next_id();
    strncpy(j.reg_no,        reg_no,        FIELD_SIZE - 1);
    strncpy(j.owner_name,    owner_name,    FIELD_SIZE - 1);
    strncpy(j.phone,         phone,         FIELD_SIZE - 1);
    strncpy(j.engine_no,     engine_no,     FIELD_SIZE - 1);
    strncpy(j.service_type,  service_type,  FIELD_SIZE - 1);
    strncpy(j.delivery_date, delivery_date, FIELD_SIZE - 1);
    strncpy(j.status,        "pending",     FIELD_SIZE - 1);
    strncpy(j.extra,         extra && *extra ? extra : "—", FIELD_SIZE - 1);
    j.priority = calc_priority(delivery_date);

    if (!fh_append(&j)) return -1;
    return j.id;
}

int jc_update(int id, const char *status, const char *delivery_date, const char *extra) {
    JobCard jobs[MAX_JOBS];
    int count = fh_read_all(jobs, MAX_JOBS);
    if (count == 0) return 0;

    int found = 0;
    for (int i = 0; i < count; i++) {
        if (jobs[i].id == id) {
            if (status && *status)
                strncpy(jobs[i].status, status, FIELD_SIZE - 1);
            if (delivery_date && *delivery_date)
                strncpy(jobs[i].delivery_date, delivery_date, FIELD_SIZE - 1);
            if (extra && *extra)
                strncpy(jobs[i].extra, extra, FIELD_SIZE - 1);
            jobs[i].priority = calc_priority(jobs[i].delivery_date);
            found = 1;
            break;
        }
    }

    if (!found) return 0;
    return fh_write_all(jobs, count);
}



int main(int argc, char *argv[]) {
    if (argc >= 2 && strcmp(argv[1], "daemon") == 0) {
        JobCard mem_jobs[MAX_JOBS];
        int mem_count = fh_read_all(mem_jobs, MAX_JOBS);
        char line[MAX_LINE];
        
        st_node_count = 1;
        memset(&st_nodes[0], 0, sizeof(STNode));
        for(int i=0; i<mem_count; i++) st_index_job(&mem_jobs[i]);
        
        while (fgets(line, sizeof(line), stdin)) {
            trim_newline(line);
            if (strcmp(line, "EXIT") == 0) {
                fh_write_all(mem_jobs, mem_count);
                break;
            } else if (strncmp(line, "ADD|", 4) == 0) {
                char *p = line + 4;
                char *fields[10];
                int nf = 0; fields[nf++] = p;
                while (*p && nf < 10) {
                    if (*p == '|') { *p = '\0'; fields[nf++] = p + 1; }
                    p++;
                }
                if (nf >= 6) { 
                    JobCard j; memset(&j, 0, sizeof(j));
                    int max_id = 0;
                    for (int i=0; i<mem_count; i++) if (mem_jobs[i].id > max_id) max_id = mem_jobs[i].id;
                    j.id = max_id + 1;
                    safe_copy(j.reg_no, fields[0], FIELD_SIZE);
                    safe_copy(j.owner_name, fields[1], FIELD_SIZE);
                    safe_copy(j.phone, fields[2], FIELD_SIZE);
                    safe_copy(j.engine_no, fields[3], FIELD_SIZE);
                    safe_copy(j.service_type, fields[4], FIELD_SIZE);
                    safe_copy(j.delivery_date, fields[5], FIELD_SIZE);
                    safe_copy(j.status, "pending", FIELD_SIZE);
                    safe_copy(j.extra, (nf >= 7 && *fields[6]) ? fields[6] : "\xE2\x80\x94", FIELD_SIZE); // Em-dash or "\xE2\x80\x94"
                    j.priority = calc_priority(j.delivery_date);
                    if (mem_count < MAX_JOBS) { 
                        mem_jobs[mem_count] = j; 
                        st_index_job(&mem_jobs[mem_count]);
                        mem_count++; 
                        printf("Added job %d successfully.\n", j.id); 
                    } else printf("Error: limit reached.\n");
                } else printf("Error: invalid add args.\n");
                fflush(stdout);
            } else if (strncmp(line, "UPDATE|", 7) == 0) {
                char *p = line + 7; char *fields[4]; int nf = 0; fields[nf++] = p;
                while (*p && nf < 4) { if (*p == '|') { *p = '\0'; fields[nf++] = p + 1; } p++; }
                if (nf >= 2) {
                    int id = atoi(fields[0]); char *status = fields[1]; char *delivery_date = (nf >= 3) ? fields[2] : ""; char *extra = (nf >= 4) ? fields[3] : "";
                    int found = 0;
                    for (int i=0; i<mem_count; i++) {
                        if (mem_jobs[i].id == id) {
                            if (status && *status) safe_copy(mem_jobs[i].status, status, FIELD_SIZE);
                            if (delivery_date && *delivery_date) safe_copy(mem_jobs[i].delivery_date, delivery_date, FIELD_SIZE);
                            if (extra && *extra) safe_copy(mem_jobs[i].extra, extra, FIELD_SIZE);
                            mem_jobs[i].priority = calc_priority(mem_jobs[i].delivery_date);
                            found = 1; break;
                        }
                    }
                    if (found) printf("Updated job %d successfully.\n", id); else printf("Error: id not found.\n");
                } else printf("Error: invalid update args.\n");
                fflush(stdout);
            } else if (strncmp(line, "SEARCH|", 7) == 0) {
                char *query = line + 7;
                int curr = 0;
                int matched = 1;
                for(int i=0; query[i] != '\0'; i++) {
                    unsigned char c = (unsigned char)query[i];
                    if (c >= 'A' && c <= 'Z') c += 32;
                    int child = st_nodes[curr].first_child;
                    int found = 0;
                    while(child != 0) {
                        if(st_nodes[child].edge_char == c) {
                            curr = child;
                            found = 1;
                            break;
                        }
                        child = st_nodes[child].next_sibling;
                    }
                    if(!found) {
                        matched = 0; break;
                    }
                }
                if (matched && curr != 0) {
                    STNode *n = &st_nodes[curr];
                    MinHeap heap;
                    heap.capacity = n->job_count;
                    heap.size = 0;
                    heap.jobs = malloc(sizeof(JobCard*) * heap.capacity);

                    for (int i = 0; i < n->job_count; i++) {
                        int j_id = n->job_ids[i];
                        for(int k=0; k < mem_count; k++) {
                            if (mem_jobs[k].id == j_id) {
                                heap_insert(&heap, &mem_jobs[k]);
                                break;
                            }
                        }
                    }
                    while(heap.size > 0) {
                        JobCard *min_job = heap_extract_min(&heap);
                        printf("%d|%s|%s|%s|%s|%s|%s|%s|%d|%s\n", min_job->id, min_job->reg_no, min_job->owner_name, min_job->phone, min_job->engine_no, min_job->service_type, min_job->delivery_date, min_job->status, min_job->priority, min_job->extra);
                    }
                    free(heap.jobs);
                }
                printf("END_SEARCH\n"); fflush(stdout);
            } else if (strcmp(line, "GET_ALL") == 0) {
                MinHeap heap;
                heap.capacity = mem_count > 0 ? mem_count : 1;
                heap.size = 0;
                heap.jobs = malloc(sizeof(JobCard*) * heap.capacity);

                for (int i=0; i<mem_count; i++) {
                    heap_insert(&heap, &mem_jobs[i]);
                }
                
                while(heap.size > 0) {
                    JobCard *min_job = heap_extract_min(&heap);
                    printf("%d|%s|%s|%s|%s|%s|%s|%s|%d|%s\n", min_job->id, min_job->reg_no, min_job->owner_name, min_job->phone, min_job->engine_no, min_job->service_type, min_job->delivery_date, min_job->status, min_job->priority, min_job->extra);
                }
                free(heap.jobs);
                
                printf("END_GET_ALL\n"); fflush(stdout);
            } else { printf("Unknown command\n"); fflush(stdout); }
        }
        return 0;
    }

    if (argc < 2) {
        printf("Usage: service.exe <command> [args]\n");
        return 1;
    }

    const char *cmd = argv[1];

    if (strcmp(cmd, "add") == 0) {
        if (argc < 8) {
            printf("Error: Missing arguments for add.\n");
            return 1;
        }
        const char *reg_no = argv[2];
        const char *owner_name = argv[3];
        const char *phone = argv[4];
        const char *engine_no = argv[5];
        const char *service_type = argv[6];
        const char *delivery_date = argv[7];
        const char *extra = (argc >= 9) ? argv[8] : "";
        
        int id = jc_add(reg_no, owner_name, phone, engine_no, service_type, delivery_date, extra);
        if (id > 0) {
            printf("Added job %d successfully.\n", id);
            return 0;
        } else {
            printf("Failed to add job.\n");
            return 1;
        }
    } 
    else if (strcmp(cmd, "update") == 0) {
        if (argc < 4) {
            printf("Error: Missing arguments for update.\n");
            return 1;
        }
        int id = atoi(argv[2]);
        const char *status = argv[3];
        const char *delivery_date = (argc >= 5) ? argv[4] : "";
        const char *extra = (argc >= 6) ? argv[5] : "";
        
        if (jc_update(id, status, delivery_date, extra)) {
            printf("Updated job %d successfully.\n", id);
            return 0;
        } else {
            printf("Failed to update job %d.\n", id);
            return 1;
        }
    }

    else {
        printf("Unknown command: %s\n", cmd);
        return 1;
    }

    return 0;
}
