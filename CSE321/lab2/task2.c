#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define NUM_STUDENTS 10
#define CHAIRS 3

sem_t chairs_sem;
sem_t st_sem;
pthread_mutex_t mutex;
int waiting_students = 0;
int total_served = 0;

typedef struct {
    int id;
} Student;

void* student_thread(void *arg) {
    Student *s = (Student*) arg;
    usleep(rand() % 1000000); // Simulate random arrival

    if (sem_trywait(&chairs_sem) == 0) {
        pthread_mutex_lock(&mutex);
        waiting_students++;
        printf("Student %d started waiting for consultation\n", s->id);
        if (waiting_students == 1) {
            sem_post(&st_sem); // Wake ST if first student
        }
        pthread_mutex_unlock(&mutex);

        // Wait for ST to serve
        sem_wait(&st_sem);
        pthread_mutex_lock(&mutex);
        waiting_students--;
        sem_post(&chairs_sem);
        printf("A waiting student started getting consultation\n");
        printf("Number of students now waiting: %d\n", waiting_students);
        pthread_mutex_unlock(&mutex);

        printf("Student %d is getting consultation\n", s->id);
        sleep(1); // Consultation time

        pthread_mutex_lock(&mutex);
        total_served++;
        printf("Student %d finished getting consultation and left\n", s->id);
        printf("Number of served students: %d\n", total_served);
        pthread_mutex_unlock(&mutex);
    } else {
        pthread_mutex_lock(&mutex);
        printf("No chairs remaining in lobby. Student %d Leaving.....\n", s->id);
        pthread_mutex_unlock(&mutex);
    }

    free(s);
    return NULL;
}

void* st_thread(void *arg) {
    while (total_served < NUM_STUDENTS) {
        sem_wait(&st_sem);
        printf("ST giving consultation\n");
        sem_post(&st_sem); // Allow next student to proceed
    }
    return NULL;
}

int main() {
    srand(time(NULL));
    sem_init(&chairs_sem, 0, CHAIRS);
    sem_init(&st_sem, 0, 0);
    pthread_mutex_init(&mutex, NULL);

    pthread_t st;
    pthread_create(&st, NULL, st_thread, NULL);

    pthread_t students[NUM_STUDENTS];
    for (int i = 0; i < NUM_STUDENTS; i++) {
        Student *s = (Student*)malloc(sizeof(Student));
        s->id = i;
        pthread_create(&students[i], NULL, student_thread, s);
    }

    for (int i = 0; i < NUM_STUDENTS; i++) {
        pthread_join(students[i], NULL);
    }

    pthread_cancel(st);
    sem_destroy(&chairs_sem);
    sem_destroy(&st_sem);
    pthread_mutex_destroy(&mutex);

    return 0;
}