#  Makefile
#  Auteur : Hacene YOUNSI
#  Email  : hacene.younsi@hotmail.com
#  Date   : 05/11/2018

SHELL = /bin/sh

# Définition des commandes utilisées
CC = gcc
CXX = g++
RM = rm -f

# Déclaration des options du compilateur
PG_FLAGS =`pkg-config --cflags --libs opencv` -lpthread
CFLAGS = -Wall -Werr
CPPFLAGS =-std=c++11

#définition des fichiers et dossiers
PROGS = run
HEADERS =
CPPSOURCES = main.cpp
OBJ  = $(CSOURCES:.cpp=.o)


all: $(PROGS)
	@echo "Exécution:"
	@./run

$(PROGS): $(OBJ)
	$(CXX) $(OBJ) $(CPPFLAGS) $(CPPSOURCES) -o $(PROGS) $(PG_FLAGS)

%.o: %.c
	@echo "Compilation: "
	$(CXX) $(CPPFLAGS) $(CPPSOURCES) -o $@  $(PG_FLAGS)

clean:
	@echo "Projet Nettoyer"
	@$(RM) -r $(PROGS) $(OBJ) *~ gmon.out core.*
