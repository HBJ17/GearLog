#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_JOBS    500
#define MAX_LINE    512
#define DATA_FILE   "../data/jobs.txt"
#define UNDO_FILE   "../data/undo.txt"
#define FIELD_SIZE  128

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

typedef struct HistoryNode {
    JobCard            job;
    struct HistoryNode *next;
} HistoryNode;

typedef struct VehicleList {
    char               reg_no[FIELD_SIZE];
    HistoryNode       *head;
    HistoryNode       *tail;
    struct VehicleList *next;
} VehicleList;

static VehicleList *vl_find_or_create(VehicleList **root, const char *reg_no) {
    VehicleList *v = *root;
    while (v) {
        if (strcmp(v->reg_no, reg_no) == 0) return v;
        v = v->next;
    }
    v = (VehicleList *)malloc(sizeof(VehicleList));
    if (!v) return NULL;
    strncpy(v->reg_no, reg_no, FIELD_SIZE - 1);
    v->reg_no[FIELD_SIZE - 1] = '\0';
    v->head = v->tail = NULL;
    v->next = *root;
    *root   = v;
    return v;
}

static void vl_append(VehicleList *v, const JobCard *j) {
    HistoryNode *node = (HistoryNode *)malloc(sizeof(HistoryNode));
    if (!node) return;
    node->job  = *j;
    node->next = NULL;
    if (!v->tail) {
        v->head = v->tail = node;
    } else {
        v->tail->next = node;
        v->tail       = node;
    }
}

static VehicleList *build_vehicle_lists(JobCard *jobs, int count) {
    VehicleList *root = NULL;
    for (int i = 0; i < count; i++) {
        VehicleList *v = vl_find_or_create(&root, jobs[i].reg_no);
        if (v) vl_append(v, &jobs[i]);
    }
    return root;
}

static void free_vehicle_lists(VehicleList *root) {
    while (root) {
        HistoryNode *h = root->head;
        while (h) {
            HistoryNode *tmp = h->next;
            free(h);
            h = tmp;
        }
        VehicleList *tmp = root->next;
        free(root);
        root = tmp;
    }
}

typedef struct StackNode {
    int  job_id;
    char field[32];
    char old_value[FIELD_SIZE];
    struct StackNode *next;
} StackNode;

static void stack_push(StackNode **top, int job_id,
                       const char *field, const char *old_value) {
    StackNode *node = (StackNode *)malloc(sizeof(StackNode));
    if (!node) return;
    node->job_id = job_id;
    strncpy(node->field,     field,     sizeof(node->field)     - 1);
    strncpy(node->old_value, old_value, sizeof(node->old_value) - 1);
    node->field[sizeof(node->field) - 1]         = '\0';
    node->old_value[sizeof(node->old_value) - 1] = '\0';
    node->next = *top;
    *top       = node;
}

static int stack_pop(StackNode **top, int *out_id,
                     char *out_field, char *out_value) {
    if (!*top) return 0;
    StackNode *node = *top;
    *out_id = node->job_id;
    strncpy(out_field, node->field,     31);
    strncpy(out_value, node->old_value, FIELD_SIZE - 1);
    out_field[31]             = '\0';
    out_value[FIELD_SIZE - 1] = '\0';
    *top = node->next;
    free(node);
    return 1;
}

static void stack_free(StackNode **top) {
    int id; char f[32], v[FIELD_SIZE];
    while (stack_pop(top, &id, f, v));
}

static void stack_save(StackNode *top) {
    StackNode *arr[MAX_JOBS];
    int n = 0;
    StackNode *cur = top;
    while (cur && n < MAX_JOBS) { arr[n++] = cur; cur = cur->next; }
    FILE *fp = fopen(UNDO_FILE, "w");
    if (!fp) return;
    for (int i = n - 1; i >= 0; i--)
        fprintf(fp, "%d|%s|%s\n", arr[i]->job_id, arr[i]->field, arr[i]->old_value);
    fclose(fp);
}

static StackNode *stack_load(void) {
    FILE *fp = fopen(UNDO_FILE, "r");
    if (!fp) return NULL;
    StackNode *top = NULL;
    char line[MAX_LINE];
    while (fgets(line, sizeof(line), fp)) {
        size_t len = strlen(line);
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r'))
            line[--len] = '\0';
        char *p1 = strchr(line, '|');
        if (!p1) continue;
        *p1 = '\0';
        char *p2 = strchr(p1 + 1, '|');
        if (!p2) continue;
        *p2 = '\0';
        int id = atoi(line);
        stack_push(&top, id, p1 + 1, p2 + 1);
    }
    fclose(fp);
    return top;
}

typedef struct QueueNode {
    int job_id;
    struct QueueNode *next;
} QueueNode;

typedef struct Queue {
    char       service_type[FIELD_SIZE];
    QueueNode *front;
    QueueNode *rear;
    struct Queue *next;
} Queue;

static Queue *queue_find_or_create(Queue **root, const char *stype) {
    Queue *q = *root;
    while (q) {
        if (strcmp(q->service_type, stype) == 0) return q;
        q = q->next;
    }
    q = (Queue *)malloc(sizeof(Queue));
    if (!q) return NULL;
    strncpy(q->service_type, stype, FIELD_SIZE - 1);
    q->service_type[FIELD_SIZE - 1] = '\0';
    q->front = q->rear = NULL;
    q->next  = *root;
    *root    = q;
    return q;
}

static void enqueue(Queue **root, const char *stype, int job_id) {
    Queue *q = queue_find_or_create(root, stype);
    if (!q) return;
    QueueNode *node = (QueueNode *)malloc(sizeof(QueueNode));
    if (!node) return;
    node->job_id = job_id;
    node->next   = NULL;
    if (!q->rear) {
        q->front = q->rear = node;
    } else {
        q->rear->next = node;
        q->rear       = node;
    }
}

static int dequeue(Queue **root, const char *stype) {
    Queue *q = *root;
    while (q) {
        if (strcmp(q->service_type, stype) == 0) break;
        q = q->next;
    }
    if (!q || !q->front) return -1;
    QueueNode *node = q->front;
    int id          = node->job_id;
    q->front        = node->next;
    if (!q->front) q->rear = NULL;
    free(node);
    return id;
}

static Queue *build_queues(JobCard *jobs, int count) {
    Queue *root = NULL;
    for (int i = 0; i < count; i++) {
        if (strcmp(jobs[i].status, "completed") != 0)
            enqueue(&root, jobs[i].service_type, jobs[i].id);
    }
    return root;
}

static void free_queues(Queue *root) {
    while (root) {
        QueueNode *n = root->front;
        while (n) { QueueNode *t = n->next; free(n); n = t; }
        Queue *t = root->next;
        free(root);
        root = t;
    }
}

static void insertion_sort(JobCard *jobs, int count) {
    for (int i = 1; i < count; i++) {
        JobCard key = jobs[i];
        int j = i - 1;
        while (j >= 0 && jobs[j].priority > key.priority) {
            jobs[j + 1] = jobs[j];
            j--;
        }
        jobs[j + 1] = key;
    }
}

static const char *job_field(const JobCard *j, const char *field) {
    if (strcmp(field, "reg_no")     == 0) return j->reg_no;
    if (strcmp(field, "owner_name") == 0) return j->owner_name;
    if (strcmp(field, "engine_no")  == 0) return j->engine_no;
    if (strcmp(field, "phone")      == 0) return j->phone;
    return "";
}

static int linear_search(const JobCard *jobs, int count,
                         const char *field, const char *query) {
    for (int i = 0; i < count; i++) {
        if (strstr(job_field(&jobs[i], field), query) != NULL)
            return i;
    }
    return -1;
}

static void linear_search_all(const JobCard *jobs, int count,
                              const char *field, const char *query) {
    int found = 0;
    for (int i = 0; i < count; i++) {
        if (strstr(job_field(&jobs[i], field), query) != NULL) {
            printf("%d|%s|%s|%s|%s|%s|%s|%s|%d|%s\n",
                jobs[i].id, jobs[i].reg_no, jobs[i].owner_name,
                jobs[i].phone, jobs[i].engine_no, jobs[i].service_type,
                jobs[i].delivery_date, jobs[i].status,
                jobs[i].priority, jobs[i].extra);
            found++;
        }
    }
    if (!found) printf("No jobs found matching %s = '%s'\n", field, query);
}

static int cmp_by_id(const void *a, const void *b) {
    return ((JobCard *)a)->id - ((JobCard *)b)->id;
}

static int binary_search_by_id(JobCard *jobs, int count, int target_id,
                                JobCard *sorted_copy) {
    memcpy(sorted_copy, jobs, count * sizeof(JobCard));
    qsort(sorted_copy, count, sizeof(JobCard), cmp_by_id);
    int lo = 0, hi = count - 1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        if (sorted_copy[mid].id == target_id) return mid;
        if (sorted_copy[mid].id  < target_id) lo = mid + 1;
        else                                   hi = mid - 1;
    }
    return -1;
}

static void trim_newline(char *s) {
    size_t len = strlen(s);
    while (len > 0 && (s[len-1] == '\n' || s[len-1] == '\r'))
        s[--len] = '\0';
}

static void safe_copy(char *dest, const char *src, int dest_size) {
    int i = 0;
    while (i < dest_size - 1 && src[i] != '\0') { dest[i] = src[i]; i++; }
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
        if (*p == '|') { *p = '\0'; fields[nf++] = p + 1; }
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
    int  count = 0;
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
            jobs[i].id, jobs[i].reg_no, jobs[i].owner_name,
            jobs[i].phone, jobs[i].engine_no, jobs[i].service_type,
            jobs[i].delivery_date, jobs[i].status,
            jobs[i].priority, jobs[i].extra);
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
    for (int i = 0; i < count; i++)
        if (jobs[i].id > max_id) max_id = jobs[i].id;
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
    strncpy(j.extra,         extra && *extra ? extra : "\xe2\x80\x94", FIELD_SIZE - 1);
    j.priority = calc_priority(delivery_date);

    if (!fh_append(&j)) return -1;

    StackNode *undo_stack = stack_load();
    stack_push(&undo_stack, j.id, "add", "");
    stack_save(undo_stack);
    stack_free(&undo_stack);

    JobCard all_jobs[MAX_JOBS];
    int count = fh_read_all(all_jobs, MAX_JOBS);
    VehicleList *vlists = build_vehicle_lists(all_jobs, count);
    free_vehicle_lists(vlists);

    return j.id;
}

int jc_update(int id, const char *status, const char *extra) {
    JobCard jobs[MAX_JOBS];
    int count = fh_read_all(jobs, MAX_JOBS);
    if (count == 0) return 0;

    JobCard sorted_copy[MAX_JOBS];
    int bi = binary_search_by_id(jobs, count, id, sorted_copy);

    int found_idx = -1;
    for (int i = 0; i < count; i++) {
        if (jobs[i].id == id) { found_idx = i; break; }
    }

    if (bi == -1 || found_idx == -1) return 0;

    StackNode *undo_stack = stack_load();
    if (status && *status)
        stack_push(&undo_stack, id, "status", jobs[found_idx].status);
    if (extra && *extra)
        stack_push(&undo_stack, id, "extra",  jobs[found_idx].extra);
    stack_save(undo_stack);
    stack_free(&undo_stack);

    if (status && *status)
        strncpy(jobs[found_idx].status, status, FIELD_SIZE - 1);
    if (extra && *extra)
        strncpy(jobs[found_idx].extra, extra, FIELD_SIZE - 1);
    jobs[found_idx].priority = calc_priority(jobs[found_idx].delivery_date);

    insertion_sort(jobs, count);

    return fh_write_all(jobs, count);
}

static int jc_undo(void) {
    StackNode *undo_stack = stack_load();
    if (!undo_stack) {
        printf("Nothing to undo.\n");
        return 0;
    }

    int  id; char field[32]; char old_value[FIELD_SIZE];
    if (!stack_pop(&undo_stack, &id, field, old_value)) {
        printf("Nothing to undo.\n");
        stack_free(&undo_stack);
        return 0;
    }

    stack_save(undo_stack);
    stack_free(&undo_stack);

    if (strcmp(field, "add") == 0) {
        JobCard jobs[MAX_JOBS];
        int count = fh_read_all(jobs, MAX_JOBS);
        int new_count = 0;
        for (int i = 0; i < count; i++) {
            if (jobs[i].id != id) jobs[new_count++] = jobs[i];
        }
        fh_write_all(jobs, new_count);
        printf("Undo successful. Removed job %d (add action reverted).\n", id);
    } else {
        JobCard jobs[MAX_JOBS];
        int count = fh_read_all(jobs, MAX_JOBS);
        int found = 0;
        for (int i = 0; i < count; i++) {
            if (jobs[i].id == id) {
                if (strcmp(field, "status") == 0)
                    strncpy(jobs[i].status, old_value, FIELD_SIZE - 1);
                else if (strcmp(field, "extra") == 0)
                    strncpy(jobs[i].extra,  old_value, FIELD_SIZE - 1);
                jobs[i].priority = calc_priority(jobs[i].delivery_date);
                found = 1;
                break;
            }
        }
        if (!found) { printf("Job %d not found for undo.\n", id); return 0; }
        insertion_sort(jobs, count);
        fh_write_all(jobs, count);
        printf("Undo successful. Reverted job %d field '%s' to '%s'.\n",
               id, field, old_value);
    }
    return 1;
}

static void jc_next_job(const char *stype) {
    JobCard jobs[MAX_JOBS];
    int count = fh_read_all(jobs, MAX_JOBS);
    Queue *queues = build_queues(jobs, count);
    int next_id = dequeue(&queues, stype);
    free_queues(queues);
    if (next_id == -1)
        printf("No pending jobs for service type '%s'.\n", stype);
    else
        printf("Next job for '%s': ID %d\n", stype, next_id);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: service.exe <command> [args]\n");
        printf("  add <reg> <owner> <phone> <engine> <service> <date> [extra]\n");
        printf("  update <id> <status> [extra]\n");
        printf("  undo\n");
        printf("  next_job <service_type>\n");
        printf("  search <field> <value>   (field: reg_no|owner_name|engine_no|phone)\n");
        return 1;
    }

    const char *cmd = argv[1];

    if (strcmp(cmd, "add") == 0) {
        if (argc < 8) { printf("Error: Missing arguments for add.\n"); return 1; }
        const char *reg_no        = argv[2];
        const char *owner_name    = argv[3];
        const char *phone         = argv[4];
        const char *engine_no     = argv[5];
        const char *service_type  = argv[6];
        const char *delivery_date = argv[7];
        const char *extra         = (argc >= 9) ? argv[8] : "";

        int id = jc_add(reg_no, owner_name, phone, engine_no,
                        service_type, delivery_date, extra);
        if (id > 0) { printf("Added job %d successfully.\n", id); return 0; }
        else        { printf("Failed to add job.\n"); return 1; }
    }

    else if (strcmp(cmd, "update") == 0) {
        if (argc < 4) { printf("Error: Missing arguments for update.\n"); return 1; }
        int id             = atoi(argv[2]);
        const char *status = argv[3];
        const char *extra  = (argc >= 5) ? argv[4] : "";

        if (jc_update(id, status, extra)) {
            printf("Updated job %d successfully.\n", id); return 0;
        } else {
            printf("Failed to update job %d.\n", id); return 1;
        }
    }

    else if (strcmp(cmd, "undo") == 0) {
        return jc_undo() ? 0 : 1;
    }

    else if (strcmp(cmd, "next_job") == 0) {
        if (argc < 3) { printf("Error: Provide a service_type.\n"); return 1; }
        jc_next_job(argv[2]);
        return 0;
    }

    else if (strcmp(cmd, "search") == 0) {
        if (argc < 4) { printf("Error: search <field> <value>\n"); return 1; }
        const char *field = argv[2];
        const char *value = argv[3];
        JobCard jobs[MAX_JOBS];
        int count = fh_read_all(jobs, MAX_JOBS);
        VehicleList *vlists = build_vehicle_lists(jobs, count);
        free_vehicle_lists(vlists);
        linear_search_all(jobs, count, field, value);
        return 0;
    }

    else {
        printf("Unknown command: %s\n", cmd); return 1;
    }

    return 0;
}
