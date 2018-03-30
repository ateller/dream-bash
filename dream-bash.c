#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <signal.h>


//Я не настолько хорошо знаю английский,
//чтобы писать на нем комментарии и не
//позориться, а исправлять перевод
//после гугла мне сейчас лень.


#define INP_SIZE 256
//Максимальная длина ввода

#ifndef IF_ERR
#define IF_ERR(check, error, info, handle)\
	do { \
		if (check == error) { \
			perror(info);\
			handle\
		} \
	} while (0)
#endif
//Макрос для обработки ошибок

#define ENV_SIZE 512
//Максимальная длина строк под
//Переменные окружения
#define INVITE "%s@%s:%s$ ", user, hostname, cwd
//То, что будет выводить prinf перед
//вводом пользователя

#define DEF_HANDL 1
#define SET_HANDL 0
//Параметры функции signals_handler
//0 - установить обрабочик, который
//будет пересылать сигналы дочернему
//процессу. 1 - вернуть дефолт

int child_pid;
//Глобальная переменная, в ней будет пид
//потомка. В него потом будут пересылаться
//сигналы

//Заменяет в пути домашний каталог на ~
void tilda(char *path, char *user)
{
	char comp[ENV_SIZE*2];
	//Здесь адрес домашнего каталога
	int offset;

	comp[0] = 0;
	strcat(comp, "/home/");
	strcat(comp, user);
	// /home/имя пользователя
	offset = 6 + strlen(user);
	//Конец нового адреса относительно
	//конца старого
	if (!strncmp(path, comp, offset)) {
		int i = 1;

		for (path[0] = '~', offset--; path[i + offset]; i++)
			path[i] = path[i + offset];
		//Ставим ~ и сдвигаем
		path[i] = 0;
	}
}

//Бьет строку на набор аргументов
char **split(char *input)
{
	char **args;
	//Он как argv
	int i = 0;

	args = malloc(sizeof(char *));
	IF_ERR(args, NULL, "Malloc error:", return NULL;);
	//Выделяем память под массив
	//аргументов
	args[i] = strtok(input, " \t\n");
	//Бьем строку
	for (; args[i] != NULL; ) {
		char **dup_args;
		//Сохраним сюда указатель перед
		//реаллоком

		i++;
		dup_args = args;
		args = realloc(args, (i + 1) * sizeof(char *));
		//Увеличиваем вектор
		if (args == NULL) {
			perror("Realloc error");
			free(dup_args);
			return NULL;
		}
		args[i] = strtok(NULL, " \t\n");
		//Записываем очередную лексему
	}
	//Когда случился NULL, выход
	return args;
}

void send_sig(int sig)
{
	if (child_pid)
		IF_ERR(kill(child_pid, sig), -1, "Kill error", ;);
	//Пересылаем сигнал потомку
}

void signals_handler(int mode)
{
	struct sigaction act;

	if (mode)
		act.sa_handler = SIG_DFL;
	else
		act.sa_handler = send_sig;
	//Вернуть деф. обработчик
	//или поставить новый
	sigemptyset(&act.sa_mask);
	//Пока не вижу необходимости
	//что-то игнорировать
	act.sa_flags = SA_RESTART;
	//Я не знаю точно, но мне
	//кажется, так будет лучше
	IF_ERR(sigaction(SIGINT, &act, NULL), -1, "Sigacion error", ;);
	IF_ERR(sigaction(SIGTSTP, &act, NULL), -1, "Sigacion error", ;);
	IF_ERR(sigaction(SIGCONT, &act, NULL), -1, "Sigacion error", ;);
	//Пересылаем сигналы прерывания,
	//приостановки и возобновления
}

void run(char **args)
{
	int i = 0;

	//for (; args[i] != NULL; i++)
	//	printf("Arg %d is %s\n", i, args[i]);
	child_pid = fork();
	//Копируем процесс
	switch (child_pid) {
	case -1:
		//если ошибка
		perror("Fork error:");
		return;
	case 0:
		i = execvp(args[0], args);
		//Запускаем программу
		if (errno != ENOENT)
			IF_ERR(i, -1, "Exec error", ;);
		else
			perror(args[0]);
		//Если такого файла нету,
		//перрор печатает
		break;
	default:
		signals_handler(SET_HANDL);
		IF_ERR(wait(&i), -1, "Wait error", kill(child_pid, 9););
		//Ждем процесс
		signals_handler(DEF_HANDL);
		printf("\n");
	}
}

//Выясним, а нет ли во вводе таких
//вещей, как cd, например. Возможно,
//что-нибудь еще, но пока только cd
void recognize(char **args)
{
	if (!strcmp(args[0], "cd")) {
	//Если во вводе cd
		if (chdir(args[1]) == -1)
			perror(args[1]);
		//Пусть cd сам решает, какой
		//путь правильный, а какой нет
		return;
		//И возвращаемся
	}
	run(args);
}

int main(int argc, char *argv[])
{
	int history;
	char buf[INP_SIZE], hostname[ENV_SIZE], *user, cwd[ENV_SIZE];
	struct passwd *userinfo;

	child_pid = 0;
	//Обнуляем пид потомка
	if (-1 == gethostname(hostname, ENV_SIZE)) {
		perror("Gethostname error");
		strcpy(hostname, "UNKNOWN_MASHINE");
	}
	//узнаем имя машины
	userinfo = getpwuid(getuid());
	if (userinfo == NULL) {
		perror("Getpwiud error");
		user = "UNKNOWN";
	} else
		user = userinfo->pw_name;
	//и имя пользователя
	history = open("history", O_RDWR|O_APPEND|O_CREAT, 0664);
	IF_ERR(history, -1, "Open error", history = -1;);

	while (true) {
		int len;
		char **args;

		IF_ERR(getcwd(cwd, ENV_SIZE), NULL, "Cwd error", exit(errno););
		//Узнаем текущий каталог
		tilda(cwd, user);
		IF_ERR(printf(INVITE), -1, "Printf error:", exit(errno););
		//приглашаем
		IF_ERR(fflush(stdout), EOF, "Fflush error:", ;);
		//чтобы вывелось сразу
		len = read(0, buf, INP_SIZE - 1);
		IF_ERR(len, -1, "Read error:", exit(errno););
		//читаем ввод
		buf[len] = 0;
		//заканчиваем строку
		//printf("Your input is: %s", buf);
		write(history, buf, len);
		//Оставим след в истории
		args = split(buf);
		//Из строки получаем массив слов
		if (split == NULL)
			continue;
		recognize(args);
		//Запускаем
		free(args);
	}
	return 0;
}
