#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h> // Добавлено для signal, kill, SIGALRM, SIGKILL

#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <getopt.h>

#include "find_min_max.h"
#include "utils.h"

// Глобальные переменные для доступа в обработчике сигнала
pid_t *child_pids = NULL;
int total_children = 0;
volatile sig_atomic_t timeout_occurred = 0;

// Обработчик сигнала SIGALRM
void alarm_handler(int signum) {
    timeout_occurred = 1;
    // Посылаем SIGKILL всем дочерним процессам
    for (int i = 0; i < total_children; i++) {
        if (child_pids[i] > 0) {
            kill(child_pids[i], SIGKILL);
        }
    }
}

int main(int argc, char **argv) {
  int seed = -1;
  int array_size = -1;
  int pnum = -1;
  bool with_files = false;
  int timeout_sec = 0; // Переменная для таймаута

  while (true) {
    int current_optind = optind ? optind : 1;

    // Добавлена опция timeout
    static struct option options[] = {{"seed", required_argument, 0, 0},
                                      {"array_size", required_argument, 0, 0},
                                      {"pnum", required_argument, 0, 0},
                                      {"by_files", no_argument, 0, 'f'},
                                      {"timeout", required_argument, 0, 0}, // Новая опция
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "f", options, &option_index);

    if (c == -1) break;

    switch (c) {
      case 0:
        switch (option_index) {
          case 0:
            seed = atoi(optarg);
            if (seed <= 0) {
              printf("seed must be a positive number\n");
              return 1;
            }
            break;
          case 1:
            array_size = atoi(optarg);
            if (array_size <= 0) {
              printf("array_size must be a positive number\n");
              return 1;
            }
            break;
          case 2:
            pnum = atoi(optarg);
            if (pnum <= 0) {
              printf("pnum must be a positive number\n");
              return 1;
            }
            break;
          case 3:
            with_files = true;
            break;
          case 4: // Обработка timeout
            timeout_sec = atoi(optarg);
            if (timeout_sec < 0) {
                printf("timeout must be non-negative\n");
                return 1;
            }
            break;

          default:
            printf("Index %d is out of options\n", option_index);
        }
        break;
      case 'f':
        with_files = true;
        break;

      case '?':
        break;

      default:
        printf("getopt returned character code 0%o?\n", c);
    }
  }

  if (optind < argc) {
    printf("Has at least one no option argument\n");
    return 1;
  }

  if (seed == -1 || array_size == -1 || pnum == -1) {
    printf("Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" [--timeout \"num\"]\n",
           argv[0]);
    return 1;
  }

  int *array = malloc(sizeof(int) * array_size);
  if (array == NULL) {
    perror("malloc");
    return 1;
  }
  GenerateArray(array, array_size, seed);
  
  // Выделяем память под массив PIDов
  child_pids = malloc(sizeof(pid_t) * pnum);
  if (child_pids == NULL) {
      perror("malloc pids");
      free(array);
      return 1;
  }
  total_children = pnum;
  
  int active_child_processes = 0;

  int pipes[2 * pnum];
  if (!with_files) {
    for (int i = 0; i < pnum; i++) {
      if (pipe(&pipes[2 * i]) == -1) {
        perror("pipe");
        free(array);
        free(child_pids);
        return 1;
      }
    }
  }

  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  // Установка обработчика сигнала и таймера
  if (timeout_sec > 0) {
      signal(SIGALRM, alarm_handler);
      alarm(timeout_sec);
  }

  for (int i = 0; i < pnum; i++) {
    pid_t child_pid = fork();
    if (child_pid >= 0) {
      active_child_processes += 1;
      if (child_pid == 0) {
        // В дочернем процессе не нужен обработчик alarm
        if (timeout_sec > 0) {
            signal(SIGALRM, SIG_DFL); 
        }

        unsigned int begin = (unsigned int)(i * array_size / pnum);
        unsigned int end   = (unsigned int)((i + 1) * array_size / pnum);

        struct MinMax result = GetMinMax(array, begin, end);

        if (with_files) {
          char filename[64];
          snprintf(filename, sizeof(filename), "child_%d.txt", i);

          FILE *f = fopen(filename, "w");
          if (!f) {
            perror("fopen");
            free(array);
            // free(child_pids); // В потомке лучше не трогать общую память без нужды, но здесь exit
            exit(1);
          }
          fprintf(f, "%d %d\n", result.min, result.max);
          fclose(f);
        } else {
          close(pipes[2 * i]);
          ssize_t written = write(pipes[2 * i + 1], &result, sizeof(result));
          if (written != sizeof(result)) {
            perror("write");
          }
          close(pipes[2 * i + 1]);

          for (int j = 0; j < pnum; j++) {
            if (j != i) {
              close(pipes[2 * j]);
              close(pipes[2 * j + 1]);
            }
          }
        }

        free(array);
        exit(0); // Используем exit вместо return в потомке
      } else {
        // Родитель сохраняет PID
        child_pids[i] = child_pid;
      }
    } else {
      printf("Fork failed!\n");
      free(array);
      free(child_pids);
      return 1;
    }
  }

  // Ожидание с использованием WNOHANG
  int status;
  while (active_child_processes > 0) {
    pid_t pid = waitpid(-1, &status, WNOHANG); // Неблокирующее ожидание
    
    if (pid > 0) {
      active_child_processes -= 1;
    } else if (pid == 0) {
      // Дети еще работают.
      // Если сработал таймаут, обработчик уже убил их.
      // Если нет, просто ждем немного (busy wait или sleep)
      if (timeout_occurred) {
          // Если таймаут был, но дети все еще не собраны waitpid (заняло время),
          // продолжаем цикл, чтобы собрать зомби.
          usleep(1000); 
      } else {
          usleep(1000); // Небольшая задержка, чтобы не грузить CPU на 100%
      }
    } else {
      // Ошибка waitpid или больше нет детей
      break;
    }
  }

  // Отключаем таймер, если он еще активен
  if (timeout_sec > 0) {
      alarm(0);
  }

  struct MinMax min_max;
  min_max.min = INT_MAX;
  min_max.max = INT_MIN;
  
  // Если таймаут произошел, результаты могут быть неполными.
  // Читаем то, что успели записать (для файлов) или прочитать (для pipe).
  // В случае pipe, если потомок убит, read вернет 0 или ошибку.

  for (int i = 0; i < pnum; i++) {
    int min = INT_MAX;
    int max = INT_MIN;
    bool data_read = false;

    if (with_files) {
      char filename[64];
      snprintf(filename, sizeof(filename), "child_%d.txt", i);

      FILE *f = fopen(filename, "r");
      if (!f) {
        // Файла нет - процесс был убит до записи
        continue;
      }
      if (fscanf(f, "%d %d", &min, &max) == 2) {
          data_read = true;
      }
      fclose(f);
      unlink(filename);
    } else {
      close(pipes[2 * i + 1]);   

      struct MinMax r;
      ssize_t n = read(pipes[2 * i], &r, sizeof(r));
      if (n == (ssize_t)sizeof(r)) {
        min = r.min;
        max = r.max;
        data_read = true;
      }
      close(pipes[2 * i]);
    }

    if (data_read) {
        if (min < min_max.min) min_max.min = min;
        if (max > min_max.max) min_max.max = max;
    }
  }

  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);

  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  free(array);
  free(child_pids);

  if (timeout_occurred) {
      printf("Timeout occurred! Results might be incomplete.\n");
  }

  printf("Min: %d\n", min_max.min);
  printf("Max: %d\n", min_max.max);
  printf("Elapsed time: %fms\n", elapsed_time);
  fflush(NULL);
  return 0;
}