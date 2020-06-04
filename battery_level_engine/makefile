CC			=	gcc

AR			=	ar rc

DEBUG	 =	true

WALL		=	true

ifndef	RELEASE
	RELEASE =	false
endif

ALLOW_LSEEK =	true

ALLOW_OPEN	=	true

OPTI_FLAG	 =	-O2

ifeq	($(WALL),true)
 CFLAGS	+=	-W -Wall -Wextra
endif

ifeq	($(RELEASE),true)
 CFLAGS	+=	$(OPTI_FLAG)
 CFLAGS	+=	-Werror
 DEBUG	=	false
else
 CFLAGS	+=	-O0
endif

ifeq	($(DEBUG),true)
 CFLAGS	+=	-g
endif

ODIR	=	objs

SDIR	=	srcs

SRC	 = main.c \
				split.c \
				battery_level_engine.c

LFLAGS	=	-lbluetooth

_OBJ	=	$(SRC:.c=.o)

OBJ	=	$(patsubst %,$(ODIR)/%,$(_OBJ))

NAME	=	battery_level_engine

all	:	mkobj $(NAME)

$(ODIR)/%.o	:	$(SDIR)/%.c
	$(CC) $(CFLAGS)	-c -o $@ $<

$(NAME)	:	$(OBJ)
	$(CC) -o $(NAME) $(OBJ) $(LFLAGS)

mkobj	:
	if [ ! -d ./$(ODIR)/ ]; then mkdir ./$(ODIR)/; fi

clean	:
	rm -f $(OBJ)

fclean	:	clean
	rm -f $(NAME)

re	:	fclean all
