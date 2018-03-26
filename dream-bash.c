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

#define INP_SIZE 256
#ifndef IF_ERR
#define IF_ERR(check, error, info)\
	do { \
		if (check == error) { \
			perror(info);\
			exit(errno);\
		} \
	} while (0)
#endif

#define ENV_SIZE 512
#define INVITE "%s@%s:%s$ ", user, hostname, cwd
//Я не настолько хорошо знаю английский,
//чтобы писать на нем комментарии и не
//позориться, а исправлять перевод
//после гугла мне сейчас лень.

void tilda(char* path, char *user)
//Заменяет в пути домашний каталог на ~
{
	char comp[ENV_SIZE*2];
	//Здесь адрес домашнего каталога
	int i = 1, offset;
	
	comp[0] = 0;
	strcat(comp, "/home/");
	strcat(comp, user);
	// /home/имя пользователя
	offset = 6 + strlen(user);
	//Конец нового адреса относительно
	//конца старого
	if (!strncmp(path, comp, offset)) {
		for(path[0] = '~', offset--; path[i + offset]; i++)
			path[i] = path [i + offset];
		//Ставим ~ и сдвигаем
		path[i] = 0;
	}		
}

int whatisthis(char *path)
{
	struct stat temp;

	if (stat(path, &temp) == -1)
		return 2;
	//Неправильный путь
	if (S_ISDIR(temp.st_mode))
		return 1;
	//Директория
	if (S_ISREG(temp.st_mode))
		return 0;
	//Файл
}

int main(int argc, char *argv[])
{
	int len;
	char buf[INP_SIZE], hostname[ENV_SIZE], *user, cwd[ENV_SIZE];
	char **paths;
	struct passwd *userinfo;

	IF_ERR(gethostname(hostname, ENV_SIZE), -1, "Can't get hostname");
	//узнаем имя машины
	userinfo = getpwuid(getuid());
	IF_ERR(userinfo, NULL, "Getpwiud error");
	user = userinfo->pw_name;
	//и имя пользователя
	IF_ERR(getcwd(cwd, ENV_SIZE), NULL, "Getcwd error");
	//Ну и текущий каталог
	tilda(cwd, user);

	while(true) {
		IF_ERR(printf(INVITE), -1, "Prinf error:");
		//приглашаем
		IF_ERR(fflush(stdout), EOF, "Fflush error:");
		//чтобы вывелось сразу
		len = read(0, buf, INP_SIZE -1);
		IF_ERR(len, -1, "Can't read your input (read error):");
		//читаем ввод
		buf[len] = 0;
		//заканчиваем строку
		printf("Your input is: %s", buf);
		
	}
}
