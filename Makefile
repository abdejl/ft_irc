NAME    = ircserv
CC      = c++
FLAGS   = -Wall -Wextra -Werror -std=c++98 -g

SRC_DIR = parsing
SRC_DIR_CHANN = channel
SRC_DIR_SERVER = server
SRCS    = $(SRC_DIR)/parser.cpp \
          $(SRC_DIR)/Client.cpp $(SRC_DIR)/cmdDispatcher.cpp\
		  $(SRC_DIR_CHANN)/channel.cpp\
		  $(SRC_DIR_SERVER)/server.cpp\
		  main.cpp

OBJS    = $(SRCS:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(FLAGS) $(OBJS) -o $(NAME)

%.o: %.cpp
	$(CC) $(FLAGS) -c $< -o $@

clean:
	rm -rf $(OBJS)

fclean: clean
	rm -rf $(NAME)

re: fclean all

.PHONY: all clean fclean re
