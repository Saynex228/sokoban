#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

#define TAILLE 12
#define MUR '#'
#define CAISSE '$'
#define VIDE ' '
#define CIBLE '.'
#define SOKOBAN '@'
#define CAISSE_CIBLE '*'
#define SOKOBAN_CIBLE '+'

typedef char t_Plateau[TAILLE][TAILLE];

int kbhit();
void chargerPartie(t_Plateau plateau, char fichier[]);
void enregistrerPartie(t_Plateau plateau, char fichier[]);

void afficher_entete(char nomFichier[], int nbDeplacement);
void afficher_plateau(t_Plateau plateau);
void deplacer(t_Plateau plateau, t_Plateau plateau_initial, char touche, int *nbDeplacements);
bool gagne(t_Plateau plateau);

void copier_plateau(t_Plateau source, t_Plateau destination);
void trouver_sokoban(t_Plateau plateau, int *y, int *x);
bool est_sur_cible(t_Plateau plateau_Initial, int y, int x);

int main() {
    t_Plateau plateau_Initial;
    t_Plateau plateau;
    char nomFichier[15];
    bool win = false;
    char touche;
    bool partieEnCours = true;
    int nbDeplacements = 0;

    printf("Tappez un nom de fichier .sok: ");
    scanf("%s", nomFichier);

    chargerPartie(plateau, nomFichier);

    copier_plateau(plateau, plateau_Initial);

    while(partieEnCours) {
        system("clear");
        afficher_entete(nomFichier, nbDeplacements);
        afficher_plateau(plateau);

        if(gagne(plateau)) {
            printf("\n=== VICTOIRE ! ===\n");
            printf("Vous avez termine en %d deplacements!\n", nbDeplacements);
            partieEnCours = false;
            win = true;
            continue;
        }

        touche = '\0';
        if(kbhit()) {
            touche = getchar();
        }

        switch(touche) {
            case 'q':
            case 'z':
            case 's':
            case 'd':
                deplacer(plateau, plateau_Initial, touche, &nbDeplacements);
                break;
            case 'x':
                printf("Voulez-vous sauvegarder (o/n): ");
                char reponse;
                scanf(" %c", &reponse);
                if(reponse == 'o' || reponse == 'O') {
                    char nomFichierSave[51];
                    printf("Entrez le nom de fichier(50 char): ");
                    scanf("%s", nomFichierSave);
                    enregistrerPartie(plateau, nomFichierSave);
                    printf("Partie sauvegardee!\n");
                }
                partieEnCours = false;
                break;
            case 'r':
                printf("\nVous voulez vraiment recommencer (o/n): ");
                char confirmation;
                scanf(" %c", &confirmation);
                if(confirmation == 'o' || confirmation == 'O') {
                    chargerPartie(plateau, nomFichier);
                    copier_plateau(plateau, plateau_Initial);
                    nbDeplacements = 0;
                    printf("Partie recommencee!\n");
                    usleep(500000);
                }
                break;
            default:
                break;
        }
    }

    if(!win) {
        printf("\n=== PARTIE ABANDONNEE ===\n");
    }

    return 0;
}

void afficher_entete(char nomFichier[], int nbDeplacements) {
    printf("==========================================================\n");
    printf("                       SOKOBAN                            \n");
    printf("==========================================================\n");
    printf("                    Partie: %s                            \n", nomFichier);
    printf("                    Deplacements: %d                      \n", nbDeplacements);
    printf("----------------------------------------------------------\n");
    printf("                       Commandes:                         \n");
    printf("  q = gauche  |  z = haut  |  s = bas  |  d = droite      \n");
    printf("          r = recommencer  |  x = abandonner              \n");
    printf("=========================================================\n\n");
}

void afficher_plateau(t_Plateau plateau) {
    char symbole;
    for(int i = 0; i < TAILLE; i++) {
        for(int j = 0; j < TAILLE; j++) {
            symbole = plateau[i][j];
            if(symbole == CAISSE_CIBLE) {
                symbole = CAISSE;
            }
            else if(symbole == SOKOBAN_CIBLE) {
                symbole = SOKOBAN;
            }
            printf("%c", symbole);
        }
        printf("\n");
    }
}

void trouver_sokoban(t_Plateau plateau, int *y, int *x) {
    for(int i = 0; i < TAILLE; i++) {
        for(int j = 0; j < TAILLE; j++) {
            if(plateau[i][j] == SOKOBAN || plateau[i][j] == SOKOBAN_CIBLE) {
                *y = i;
                *x = j;
                return;
            }
        }
    }
}

bool est_sur_cible(t_Plateau plateau_Initial, int y, int x) {
    char symbole = plateau_Initial[y][x];
    return (symbole == CIBLE || symbole == CAISSE_CIBLE || symbole == SOKOBAN_CIBLE);
}

void deplacer(t_Plateau plateau, t_Plateau plateau_Initial, char touche, int *nbDeplacements) {
    int sokobanY, sokobanX;
    trouver_sokoban(plateau, &sokobanY, &sokobanX);
    int dx = 0;
    int dy = 0;

    switch(touche) {
        case 'q': dx = -1; break;
        case 'd': dx = +1; break;
        case 'z': dy = -1; break;
        case 's': dy = +1; break;
        default: return;
    }

    int nouvelleX = sokobanX + dx;
    int nouvelleY = sokobanY + dy;

    if(nouvelleX < 0 || nouvelleX >= TAILLE || nouvelleY < 0 || nouvelleY >= TAILLE) {
        return;
    }

    char destination = plateau[nouvelleY][nouvelleX];

    if(destination == MUR) {
        return;
    }
    else if(destination == VIDE || destination == CIBLE) {
        if(est_sur_cible(plateau_Initial, sokobanY, sokobanX)) {
            plateau[sokobanY][sokobanX] = CIBLE;
        }
        else {
            plateau[sokobanY][sokobanX] = VIDE;
        }

        if(destination == CIBLE) {
            plateau[nouvelleY][nouvelleX] = SOKOBAN_CIBLE;
        }
        else {
            plateau[nouvelleY][nouvelleX] = SOKOBAN;
        }
        (*nbDeplacements)++;
    }
    else if(destination == CAISSE || destination == CAISSE_CIBLE) {
        int nouvelleCaisseY = nouvelleY + dy;
        int nouvelleCaisseX = nouvelleX + dx;

        if(nouvelleCaisseX < 0 || nouvelleCaisseX >= TAILLE || nouvelleCaisseY < 0 || nouvelleCaisseY >= TAILLE) {
            return;
        }

        char derriereCaisse = plateau[nouvelleCaisseY][nouvelleCaisseX];

        if(derriereCaisse == VIDE || derriereCaisse == CIBLE) {
            if(derriereCaisse == CIBLE) {
                plateau[nouvelleCaisseY][nouvelleCaisseX] = CAISSE_CIBLE;
            }
            else {
                plateau[nouvelleCaisseY][nouvelleCaisseX] = CAISSE;
            }

            if(destination == CAISSE_CIBLE) {
                plateau[nouvelleY][nouvelleX] = SOKOBAN_CIBLE;
            }
            else {
                plateau[nouvelleY][nouvelleX] = SOKOBAN;
            }

            if(est_sur_cible(plateau_Initial, sokobanY, sokobanX)) {
                plateau[sokobanY][sokobanX] = CIBLE;
            }
            else {
                plateau[sokobanY][sokobanX] = VIDE;
            }
            (*nbDeplacements)++;
        }
    }
}

bool gagne(t_Plateau plateau) {
    for(int i = 0; i < TAILLE; i++) {
        for(int j = 0; j < TAILLE; j++) {
            if(plateau[i][j] == CAISSE) {
                return false;
            }
        }
    }
    return true;
}

void copier_plateau(t_Plateau source, t_Plateau destination) {
    for(int i = 0; i < TAILLE; i++) {
        for(int j = 0; j < TAILLE; j++) {
            destination[i][j] = source[i][j];
        }
    }
}

int kbhit() {
    int unCaractere = 0;
    struct termios oldt, newt;
    int ch;
    int oldf;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);

    if(ch != EOF) {
        ungetc(ch, stdin);
        unCaractere = 1;
    }
    return unCaractere;
}

void chargerPartie(t_Plateau plateau, char fichier[]) {
    FILE * f;
    char finDeLigne;

    f = fopen(fichier, "r");
    if(f == NULL) {
        printf("ERREUR SUR FICHIER\n");
        exit(EXIT_FAILURE);
    } else {
        for(int ligne = 0; ligne < TAILLE; ligne++) {
            for(int colonne = 0; colonne < TAILLE; colonne++) {
                fread(&plateau[ligne][colonne], sizeof(char), 1, f);
            }
            fread(&finDeLigne, sizeof(char), 1, f);
        }
        fclose(f);
    }
}

void enregistrerPartie(t_Plateau plateau, char fichier[]) {
    FILE * f;
    char finDeLigne = '\n';

    f = fopen(fichier, "w");
    for(int ligne = 0; ligne < TAILLE; ligne++) {
        for(int colonne = 0; colonne < TAILLE; colonne++) {
            fwrite(&plateau[ligne][colonne], sizeof(char), 1, f);
        }
        fwrite(&finDeLigne, sizeof(char), 1, f);
    }
    fclose(f);
}
