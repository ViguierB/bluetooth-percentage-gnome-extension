CC			=	gcc

AR			=	ar rc

DEBUG	 =	true

WALL		=	true

ifndef	RELEASE
	RELEASE =	false
endif

OPTI_FLAG	 =	-O2

ifeq	($(WALL),true)
 CFLAGS	+=	-W -Wall -Wextra
endif

ifeq	($(RELEASE),true)
 CFLAGS	+=	$(OPTI_FLAG)
 CFLAGS	+=	-Werror -DNDEBUG -fdata-sections -ffunction-sections
 LFLAGS +=	-Wl,--gc-sections -Wl,--strip-all
 DEBUG	=	false
else
 CFLAGS	+=	-O0
endif

ifeq	($(DEBUG),true)
 CFLAGS	+=	-g
endif

ODIR	=	objs

SDIR	=	srcs

SRC	 =	main.c \
				misc.c \
				io_context.c \
				signals.c	\
				battery_level_engine.c

LFLAGS	+=	-lbluetooth -lpthread

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
