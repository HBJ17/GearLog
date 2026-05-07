#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

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
    char payment_status[FIELD_SIZE]; 
} JobCard;

// --- UTILITY FUNCTIONS ---
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

void str_tolower(const char* src, char* dest) {
    int i = 0;
    while(src[i]) {
        dest[i] = tolower((unsigned char)src[i]);
        i++;
    }
    dest[i] = '\0';
}

// --- SUFFIX TREE FOR SEARCH ---
typedef struct SuffixNode {
    struct SuffixNode* children[256];
    int job_ids[MAX_JOBS];
    int job_count;
} SuffixNode;

SuffixNode* suffix_root = NULL;

SuffixNode* create_node() {
    SuffixNode* node = (SuffixNode*)calloc(1, sizeof(SuffixNode));
    return node;
}

void insert_suffix(const char* str, int job_id) {
    int i, j, k, len;
    if (!str) return;
    len = strlen(str);
    for (i = 0; i < len; i++) {
        SuffixNode* curr = suffix_root;
        for (j = i; j < len; j++) {
            unsigned char c = (unsigned char)tolower(str[j]);
            int exists = 0;
            if (!curr->children[c]) curr->children[c] = create_node();
            curr = curr->children[c];
            for (k = 0; k < curr->job_count; k++) {
                if (curr->job_ids[k] == job_id) { exists = 1; break; }
            }
            if (!exists && curr->job_count < MAX_JOBS) {
                curr->job_ids[curr->job_count++] = job_id;
            }
        }
    }
}

void build_index(JobCard* jobs, int count) {
    int i;
    suffix_root = create_node();
    for (i = 0; i < count; i++) {
        insert_suffix(jobs[i].reg_no, jobs[i].id);
        insert_suffix(jobs[i].phone, jobs[i].id);
    }
}

// --- FILE HANDLING ---
static int parse_line(const char *line, JobCard *j) {
    char buf[MAX_LINE];
    char *fields[11];
    int nf = 0;
    char *p;
    strncpy(buf, line, MAX_LINE - 1);
    buf[MAX_LINE - 1] = '\0';
    trim_newline(buf);
    p = buf;
    fields[nf++] = p;
    while (*p && nf < 11) {
        if (*p == '|') { *p = '\0'; fields[nf++] = p + 1; }
        p++;
    }
    if (nf < 10) return 0; 
    j->id = atoi(fields[0]);
    safe_copy(j->reg_no, fields[1], FIELD_SIZE);
    safe_copy(j->owner_name, fields[2], FIELD_SIZE);
    safe_copy(j->phone, fields[3], FIELD_SIZE);
    safe_copy(j->engine_no, fields[4], FIELD_SIZE);
    safe_copy(j->service_type, fields[5], FIELD_SIZE);
    safe_copy(j->delivery_date, fields[6], FIELD_SIZE);
    safe_copy(j->status, fields[7], FIELD_SIZE);
    j->priority = atoi(fields[8]);
    safe_copy(j->extra, fields[9], FIELD_SIZE);
    if (nf == 11) safe_copy(j->payment_status, fields[10], FIELD_SIZE);
    else safe_copy(j->payment_status, "unpaid", FIELD_SIZE);
    return 1;
}

int fh_read_all(JobCard *jobs, int max) {
    FILE *fp = fopen(DATA_FILE, "r");
    int count = 0;
    char line[MAX_LINE];
    if (!fp) return 0;
    while (count < max && fgets(line, sizeof(line), fp)) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;
        if (parse_line(line, &jobs[count])) count++;
    }
    fclose(fp);
    build_index(jobs, count);
    return count;
}

int fh_write_all(JobCard *jobs, int count) {
    int i;
    FILE *fp = fopen(DATA_FILE, "w");
    if (!fp) return 0;
    fprintf(fp, "# ID|RegNo|OwnerName|Phone|EngineNo|ServiceType|DeliveryDate|Status|Priority|Extra|PaymentStatus\n");
    for (i = 0; i < count; i++) {
        fprintf(fp, "%d|%s|%s|%s|%s|%s|%s|%s|%d|%s|%s\n",
            jobs[i].id, jobs[i].reg_no, jobs[i].owner_name, jobs[i].phone,
            jobs[i].engine_no, jobs[i].service_type, jobs[i].delivery_date,
            jobs[i].status, jobs[i].priority, jobs[i].extra, jobs[i].payment_status);
    }
    fclose(fp);
    return 1;
}

int main(int argc, char *argv[]) {
    if (argc >= 2 && strcmp(argv[1], "daemon") == 0) {
        JobCard mem_jobs[MAX_JOBS];
        int mem_count = fh_read_all(mem_jobs, MAX_JOBS);
        char line[MAX_LINE];
        int i, j, k; // All loop variables declared here
        
        while (fgets(line, sizeof(line), stdin)) {
            trim_newline(line);
            if (strcmp(line, "EXIT") == 0) {
                fh_write_all(mem_jobs, mem_count);
                break;
            } 
            else if (strncmp(line, "SEARCH|", 7) == 0) {
                char* query = line + 7;
                char lower_query[MAX_LINE];
                str_tolower(query, lower_query);
                if (lower_query[0] != '\0' && suffix_root != NULL) {
                    SuffixNode* curr = suffix_root;
                    int found_match = 1;
                    for (j = 0; lower_query[j] != '\0'; j++) {
                        unsigned char c = (unsigned char)lower_query[j];
                        if (curr->children[c] == NULL) { found_match = 0; break; }
                        curr = curr->children[c];
                    }
                    if (found_match) {
                        for (j = 0; j < curr->job_count; j++) {
                            int jid = curr->job_ids[j];
                            for (k = 0; k < mem_count; k++) {
                                if (mem_jobs[k].id == jid) {
                                    printf("%d|%s|%s|%s|%s|%s|%s|%s|%d|%s|%s\n", 
                                        mem_jobs[k].id, mem_jobs[k].reg_no, mem_jobs[k].owner_name, 
                                        mem_jobs[k].phone, mem_jobs[k].engine_no, mem_jobs[k].service_type, 
                                        mem_jobs[k].delivery_date, mem_jobs[k].status, mem_jobs[k].priority, 
                                        mem_jobs[k].extra, mem_jobs[k].payment_status);
                                    break;
                                }
                            }
                        }
                    }
                }
                printf("END_SEARCH\n");
                fflush(stdout);
            }
            else if (strncmp(line, "UPDATE_PAYMENT|", 15) == 0) {
                char *p = line + 15;
                char *fields[2]; int nf = 0;
                fields[nf++] = p;
                while (*p && nf < 2) { if (*p == '|') { *p = '\0'; fields[nf++] = p + 1; } p++; }
                if (nf >= 2) {
                    int id = atoi(fields[0]);
                    for (i = 0; i < mem_count; i++) {
                        if (mem_jobs[i].id == id) {
                            safe_copy(mem_jobs[i].payment_status, fields[1], FIELD_SIZE);
                            break;
                        }
                    }
                    fh_write_all(mem_jobs, mem_count);
                    printf("successfully updated payment\n");
                }
                fflush(stdout);
            }
            else if (strcmp(line, "GET_ALL") == 0) {
                for (i = 0; i < mem_count; i++) {
                    printf("%d|%s|%s|%s|%s|%s|%s|%s|%d|%s|%s\n", 
                        mem_jobs[i].id, mem_jobs[i].reg_no, mem_jobs[i].owner_name, 
                        mem_jobs[i].phone, mem_jobs[i].engine_no, mem_jobs[i].service_type, 
                        mem_jobs[i].delivery_date, mem_jobs[i].status, mem_jobs[i].priority, 
                        mem_jobs[i].extra, mem_jobs[i].payment_status);
                }
                printf("END_GET_ALL\n");
                fflush(stdout);
            }
        }
    }
    return 0;
}
