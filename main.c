#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "cJSON.h"

#define MAX_USERS 100
#define BUFFER_SIZE 8192

typedef struct {
    long chat_id;
    char file_name[256];
    char content[BUFFER_SIZE];
    int active;
} UserData;

UserData users[MAX_USERS];

const char *ALLOWED[] = {"py","java","c","cpp","txt"};
const int ALLOWED_COUNT = 5;

char *TOKEN;
long last_update_id = 0;

struct Memory {
    char *response;
    size_t size;
};

size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t total = size * nmemb;
    struct Memory *mem = (struct Memory *)userp;

    mem->response = realloc(mem->response, mem->size + total + 1);
    memcpy(&(mem->response[mem->size]), contents, total);
    mem->size += total;
    mem->response[mem->size] = 0;

    return total;
}

void send_message(long chat_id, const char *text) {
    CURL *curl = curl_easy_init();
    if (!curl) return;

    char url[1024];
    snprintf(url, sizeof(url),
        "https://api.telegram.org/bot%s/sendMessage",
        TOKEN);

    char post_data[2048];
    snprintf(post_data, sizeof(post_data),
        "chat_id=%ld&text=%s", chat_id, text);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);
    curl_easy_perform(curl);
    curl_easy_cleanup(curl);
}

int is_allowed_extension(const char *filename) {
    char *dot = strrchr(filename, '.');
    if (!dot) return 0;
    dot++;

    for (int i = 0; i < ALLOWED_COUNT; i++) {
        if (strcmp(dot, ALLOWED[i]) == 0)
            return 1;
    }
    return 0;
}

UserData* get_user(long chat_id) {
    for (int i = 0; i < MAX_USERS; i++) {
        if (users[i].active && users[i].chat_id == chat_id)
            return &users[i];
    }

    for (int i = 0; i < MAX_USERS; i++) {
        if (!users[i].active) {
            users[i].active = 1;
            users[i].chat_id = chat_id;
            users[i].content[0] = '\0';
            return &users[i];
        }
    }

    return NULL;
}

void handle_message(long chat_id, const char *text) {
    UserData *user = get_user(chat_id);

    if (strcmp(text, "/start") == 0) {
        send_message(chat_id,
            "Привет! Отправь имя файла с расширением, затем содержимое.");
        return;
    }

    if (user->file_name[0] == '\0') {
        if (!is_allowed_extension(text)) {
            send_message(chat_id, "Неверный формат или расширение.");
            return;
        }
        strcpy(user->file_name, text);
        send_message(chat_id, "Имя файла принято. Теперь отправляй текст.");
        return;
    }

    if (strcmp(text, "Готово") == 0 || strcmp(text, "/end") == 0) {
        FILE *f = fopen(user->file_name, "w");
        if (f) {
            fprintf(f, "%s", user->content);
            fclose(f);
        }

        send_message(chat_id, "Файл сохранён (отправка файла не реализована).");

        user->file_name[0] = '\0';
        user->content[0] = '\0';
        return;
    }

    strcat(user->content, text);
    strcat(user->content, "\n");
    send_message(chat_id, "Текст добавлен.");
}

void poll_updates() {
    CURL *curl = curl_easy_init();
    if (!curl) return;

    struct Memory chunk = {0};

    char url[1024];
    snprintf(url, sizeof(url),
        "https://api.telegram.org/bot%s/getUpdates?offset=%ld&timeout=30",
        TOKEN, last_update_id + 1);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &chunk);
    curl_easy_perform(curl);

    cJSON *json = cJSON_Parse(chunk.response);
    if (!json) return;

    cJSON *result = cJSON_GetObjectItem(json, "result");
    int size = cJSON_GetArraySize(result);

    for (int i = 0; i < size; i++) {
        cJSON *update = cJSON_GetArrayItem(result, i);
        cJSON *update_id = cJSON_GetObjectItem(update, "update_id");
        last_update_id = update_id->valuedouble;

        cJSON *message = cJSON_GetObjectItem(update, "message");
        if (!message) continue;

        cJSON *chat = cJSON_GetObjectItem(message, "chat");
        long chat_id = cJSON_GetObjectItem(chat, "id")->valuedouble;

        cJSON *text = cJSON_GetObjectItem(message, "text");
        if (!text) continue;

        handle_message(chat_id, text->valuestring);
    }

    cJSON_Delete(json);
    free(chunk.response);
    curl_easy_cleanup(curl);
}

int main() {
    TOKEN = getenv("TOKEN");
    if (!TOKEN) {
        printf("TOKEN not set\n");
        return 1;
    }

    curl_global_init(CURL_GLOBAL_DEFAULT);

    while (1) {
        poll_updates();
    }

    curl_global_cleanup();
    return 0;
}