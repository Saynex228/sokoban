/*
 * Auteur : Tuila Abdelkarim
 * Date : 06/11/2025
 * Description : Jeu Sokoban avec zoom, historique et annulation
 * Version : 2.0
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

/* Constantes pour le plateau */
#define TAILLE_DEPLACEMENT 1000
#define TAILLE 12
#define MUR '#'
#define CAISSE '$'
#define VIDE ' '
#define CIBLE '.'
#define SOKOBAN '@'
#define CAISSE_CIBLE '*'
#define SOKOBAN_CIBLE '+'

/* Types personnalisés */
typedef char t_tabDeplacement[TAILLE_DEPLACEMENT];
typedef char t_Plateau[TAILLE][TAILLE];

/* Les fonctions fournies par l'université */
int kbhit();
void chargerPartie(t_Plateau plateau, char fichier[]);
void enregistrerPartie(t_Plateau plateau, char fichier[]);
void enregistrerDeplacements(t_tabDeplacement t, int nb, char fic[]);

/* Les fonctions obligatoires */
void afficher_entete(t_tabDeplacement tab, char nomFichier[],
                     int nbDeplacements);
void afficher_plateau(t_Plateau plateau, int zoom);
void deplacer(t_tabDeplacement tab, t_Plateau plateau,
              t_Plateau plateau_initial, char touche,
              int *nbDeplacements);
bool gagne(t_Plateau plateau);
void undo_deplacements(t_tabDeplacement tab, t_Plateau plateau,
                       t_Plateau plateau_initial, int *nbDeplacements);

/* Les fonctions auxiliaires */
void copier_plateau(t_Plateau source, t_Plateau destination);
void trouver_sokoban(t_Plateau plateau, int *y, int *x);
bool est_sur_cible(t_Plateau plateau_initial, int y, int x);
void traiter_victoire(t_tabDeplacement tab, int nbDeplacements, bool *win,
                      bool *partieEnCours);
void traiter_abandon(t_Plateau plateau, bool *partieEnCours);
void traiter_recommencer(t_Plateau plateau, t_Plateau plateauInitial,
                         char nomFichier[], int *nbDeplacements);
void sauvegarder_deplacements_fin(t_tabDeplacement tab, int nbDeplacements);
void deplacer_simple(t_Plateau plateau, t_Plateau plateauInitial,
                     int sokobanY, int sokobanX, int nouvelleY,
                     int nouvelleX, char destination);
void deplacer_caisse(t_Plateau plateau, t_Plateau plateauInitial,
                     int sokobanY, int sokobanX, int nouvelleY,
                     int nouvelleX, int dy, int dx, char destination);
void restaurer_position_sokoban(t_Plateau plateau, t_Plateau plateauInitial,
                                int sokobanY, int sokobanX);
void placer_sokoban(t_Plateau plateau, t_Plateau plateauInitial,
                    int y, int x);

/*
 * Fonction principale du jeu Sokoban
 * Gère la boucle de jeu et les interactions utilisateur
 */
int main() {
    t_Plateau plateauInitial;
    t_Plateau plateau;
    t_tabDeplacement tab;
    char nomFichier[15];
    bool win = false;
    char touche;
    bool partieEnCours = true;
    int nbDeplacements = 0;
    int zoom = 1;

    printf("Tappez un nom de fichier : ");
    scanf("%s", nomFichier);

    chargerPartie(plateau, nomFichier);
    copier_plateau(plateau, plateauInitial);

    while(partieEnCours) {
        system("clear");
        afficher_entete(tab, nomFichier, nbDeplacements);
        afficher_plateau(plateau, zoom);

        if(gagne(plateau)) {
            traiter_victoire(tab, nbDeplacements, &win, &partieEnCours);
            continue;
        }

        touche = '\0';
        if(kbhit()) {
            scanf("%c", &touche);
        }

        switch(touche) {
            case 'q':
            case 'z':
            case 's':
            case 'd':
                deplacer(tab, plateau, plateauInitial, touche,
                         &nbDeplacements);
                break;
            case 'x':
                traiter_abandon(plateau, &partieEnCours);
                break;
            case 'r':
                traiter_recommencer(plateau, plateauInitial, nomFichier,
                                    &nbDeplacements);
                break;
            case '+':
                if(zoom < 3) zoom++;
                break;
            case '-':
                if(zoom > 1) zoom--;
                break;
            case 'u':
                undo_deplacements(tab, plateau, plateauInitial,
                                  &nbDeplacements);
                break;
            default:
                break;
        }
    }

    if(!win) {
        sauvegarder_deplacements_fin(tab, nbDeplacements);
        printf("\n=== PARTIE ABANDONNEE ===\n");
    }

    return 0;
}

/*
 * Affiche l'en-tete du jeu avec les informations et les commandes
 */
void afficher_entete(t_tabDeplacement tab, char nomFichier[],
                     int nbDeplacements) {
    printf("====================================================\n");
    printf("                       SOKOBAN\n");
    printf("====================================================\n");
    printf("                    Partie : %s\n", nomFichier);
    printf("                    Deplacements : %d\n", nbDeplacements);
    printf("----------------------------------------------------\n");
    printf("                       Commandes\n");
    printf("  q = gauche  |  z = haut  |  s = bas  |  d = droite\n");
    printf("          r = recommencer  |  x = abandonner\n");
    printf("              + = zoomer  |  - = dezoomer\n");
    printf("                       u = annuler\n");
    printf("Histoire des deplacements: ");
    for(int i = 0; i < nbDeplacements; i++) {
        printf(" %c", tab[i]);
    }
    printf("\n====================================================\n");
}

/*
 * Affiche le plateau de jeu avec le niveau de zoom specifie
 */
void afficher_plateau(t_Plateau plateau, int zoom) {
    char symbole;
    for(int i = 0; i < TAILLE; i++) {
        for(int iz = 0; iz < zoom; iz++) {
            for(int j = 0; j < TAILLE; j++) {
                for(int ij = 0; ij < zoom; ij++) {
                    symbole = plateau[i][j];
                    if(symbole == CAISSE_CIBLE) {
                        symbole = CAISSE;
                    }
                    else if(symbole == SOKOBAN_CIBLE) {
                        symbole = SOKOBAN;
                    }
                    printf("%c", symbole);
                }
            }
            printf("\n");
        }
    }
}

/*
 * Trouve la position du Sokoban sur le plateau
 */
void trouver_sokoban(t_Plateau plateau, int *y, int *x) {
    for(int i = 0; i < TAILLE; i++) {
        for(int j = 0; j < TAILLE; j++) {
            if(plateau[i][j] == SOKOBAN ||
               plateau[i][j] == SOKOBAN_CIBLE) {
                *y = i;
                *x = j;
                return;
            }
        }
    }
}

/*
 * Verifie si une position correspond a une cible sur le plateau initial
 */
bool est_sur_cible(t_Plateau plateau_initial, int y, int x) {
    char symbole = plateau_initial[y][x];
    return (symbole == CIBLE || symbole == CAISSE_CIBLE ||
            symbole == SOKOBAN_CIBLE);
}

/*
 * Restaure la case de depart du Sokoban apres deplacement
 */
void restaurer_position_sokoban(t_Plateau plateau, t_Plateau plateauInitial,
                                int sokobanY, int sokobanX) {
    if(est_sur_cible(plateauInitial, sokobanY, sokobanX)) {
        plateau[sokobanY][sokobanX] = CIBLE;
    }
    else {
        plateau[sokobanY][sokobanX] = VIDE;
    }
}

/*
 * Place le Sokoban a une nouvelle position
 */
void placer_sokoban(t_Plateau plateau, t_Plateau plateauInitial,
                    int y, int x) {
    if(est_sur_cible(plateauInitial, y, x)) {
        plateau[y][x] = SOKOBAN_CIBLE;
    }
    else {
        plateau[y][x] = SOKOBAN;
    }
}

/*
 * Deplace le Sokoban seul sans caisse
 */
void deplacer_simple(t_Plateau plateau, t_Plateau plateauInitial,
                     int sokobanY, int sokobanX, int nouvelleY,
                     int nouvelleX, char destination) {
    restaurer_position_sokoban(plateau, plateauInitial, sokobanY, sokobanX);
    placer_sokoban(plateau, plateauInitial, nouvelleY, nouvelleX);
}

/*
 * Deplace le Sokoban et une caisse
 */
void deplacer_caisse(t_Plateau plateau, t_Plateau plateauInitial,
                     int sokobanY, int sokobanX, int nouvelleY,
                     int nouvelleX, int dy, int dx, char destination) {
    int nouvelleCaisseY = nouvelleY + dy;
    int nouvelleCaisseX = nouvelleX + dx;

    if(nouvelleCaisseX < 0 || nouvelleCaisseX >= TAILLE ||
       nouvelleCaisseY < 0 || nouvelleCaisseY >= TAILLE) {
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

        placer_sokoban(plateau, plateauInitial, nouvelleY, nouvelleX);
        restaurer_position_sokoban(plateau, plateauInitial,
                                   sokobanY, sokobanX);
    }
}

/*
 * Deplace le Sokoban selon la touche pressee et memorise le deplacement
 */
void deplacer(t_tabDeplacement tab, t_Plateau plateau,
              t_Plateau plateauInitial, char touche,
              int *nbDeplacements) {
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

    if(nouvelleX < 0 || nouvelleX >= TAILLE ||
       nouvelleY < 0 || nouvelleY >= TAILLE) {
        return;
    }

    char destination = plateau[nouvelleY][nouvelleX];

    if(destination == MUR) {
        return;
    }
    else if(destination == VIDE || destination == CIBLE) {
        deplacer_simple(plateau, plateauInitial, sokobanY, sokobanX,
                        nouvelleY, nouvelleX, destination);
        (*nbDeplacements)++;
        tab[*nbDeplacements - 1] = touche;
    }
    else if(destination == CAISSE || destination == CAISSE_CIBLE) {
        deplacer_caisse(plateau, plateauInitial, sokobanY, sokobanX,
                        nouvelleY, nouvelleX, dy, dx, destination);

        if(plateau[nouvelleY][nouvelleX] == SOKOBAN ||
           plateau[nouvelleY][nouvelleX] == SOKOBAN_CIBLE) {
            (*nbDeplacements)++;
            if(touche >= 'a' && touche <= 'z') {
                touche = touche - ('a' - 'A');
            }
            tab[*nbDeplacements - 1] = touche;
        }
    }
}

/*
 * Annule le dernier deplacement effectue
 */
void undo_deplacements(t_tabDeplacement tab, t_Plateau plateau,
                       t_Plateau plateauInitial, int *nbDeplacements) {
    if(*nbDeplacements == 0) return;

    char touche = tab[*nbDeplacements - 1];
    int sokobanY, sokobanX;
    int dx = 0, dy = 0;

    trouver_sokoban(plateau, &sokobanY, &sokobanX);

    switch(touche) {
        case 'q': case 'Q': dx = +1; break;
        case 'd': case 'D': dx = -1; break;
        case 'z': case 'Z': dy = +1; break;
        case 's': case 'S': dy = -1; break;
        default: return;
    }

    int nouvelleX = sokobanX + dx;
    int nouvelleY = sokobanY + dy;
    int caisseY = sokobanY - dy;
    int caisseX = sokobanX - dx;

    if(nouvelleX < 0 || nouvelleX >= TAILLE ||
       nouvelleY < 0 || nouvelleY >= TAILLE) {
        return;
    }

    if(touche >= 'A' && touche <= 'Z') {
        if(est_sur_cible(plateauInitial, sokobanY, sokobanX)) {
            plateau[sokobanY][sokobanX] = CAISSE_CIBLE;
        }
        else {
            plateau[sokobanY][sokobanX] = CAISSE;
        }

        if(est_sur_cible(plateauInitial, caisseY, caisseX)) {
            plateau[caisseY][caisseX] = CIBLE;
        }
        else {
            plateau[caisseY][caisseX] = VIDE;
        }
    }
    else {
        if(est_sur_cible(plateauInitial, sokobanY, sokobanX)) {
            plateau[sokobanY][sokobanX] = CIBLE;
        }
        else {
            plateau[sokobanY][sokobanX] = VIDE;
        }
    }

    if(est_sur_cible(plateauInitial, nouvelleY, nouvelleX)) {
        plateau[nouvelleY][nouvelleX] = SOKOBAN_CIBLE;
    }
    else {
        plateau[nouvelleY][nouvelleX] = SOKOBAN;
    }

    (*nbDeplacements)--;
}

/*
 * Verifie si toutes les caisses sont placees sur les cibles
 */
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

/*
 * Copie le contenu d'un plateau dans un autre
 */
void copier_plateau(t_Plateau source, t_Plateau destination) {
    for(int i = 0; i < TAILLE; i++) {
        for(int j = 0; j < TAILLE; j++) {
            destination[i][j] = source[i][j];
        }
    }
}

/*
 * Traite la victoire du joueur et propose de sauvegarder
 */
void traiter_victoire(t_tabDeplacement tab, int nbDeplacements, bool *win,
                      bool *partieEnCours) {
    printf("\n=== VICTOIRE ! ===\n");
    printf("Vous avez termine le niveau !\n");
    printf("Souhaitez-vous enregistrer les deplacements? (o/n): ");
    char repondre;
    scanf(" %c", &repondre);
    if(repondre == 'o' || repondre == 'O') {
        char nomFichierDeplacements[50];
        printf("Entrez le nom du fichier .dep : ");
        scanf(" %s", nomFichierDeplacements);
        enregistrerDeplacements(tab, nbDeplacements,
                                nomFichierDeplacements);
        printf("Deplacements sauvegardes !\n");
    }
    *partieEnCours = false;
    *win = true;
}

/*
 * Traite l'abandon de la partie et propose de sauvegarder
 */
void traiter_abandon(t_Plateau plateau, bool *partieEnCours) {
    printf("Voulez-vous sauvegarder la partie? (o/n): ");
    char reponse;
    scanf(" %c", &reponse);
    if(reponse == 'o' || reponse == 'O') {
        char nomFichierSauvegarde[50];
        printf("\nEntrez le nom du fichier : ");
        scanf("%s", nomFichierSauvegarde);
        enregistrerPartie(plateau, nomFichierSauvegarde);
        printf("Partie sauvegardee !\n");
    }
    *partieEnCours = false;
}

/*
 * Recommence la partie en rechargeant le niveau initial
 */
void traiter_recommencer(t_Plateau plateau, t_Plateau plateauInitial,
                         char nomFichier[], int *nbDeplacements) {
    printf("\nVous voulez recommencer? (o/n): ");
    char confirmation;
    scanf(" %c", &confirmation);
    if(confirmation == 'o' || confirmation == 'O') {
        chargerPartie(plateau, nomFichier);
        copier_plateau(plateau, plateauInitial);
        *nbDeplacements = 0;
        printf("Partie recommencee !\n");
    }
}

/*
 * Propose de sauvegarder les deplacements en fin de partie
 */
void sauvegarder_deplacements_fin(t_tabDeplacement tab, int nbDeplacements) {
    printf("Souhaitez-vous enregistrer les deplacements? (o/n): ");
    char confirmation;
    scanf(" %c", &confirmation);
    if(confirmation == 'o' || confirmation == 'O') {
        char nomFichierDeplacements[50];
        printf("Entrez le nom de fichier .dep : ");
        scanf(" %50s", nomFichierDeplacements);
        enregistrerDeplacements(tab, nbDeplacements,
                                nomFichierDeplacements);
        printf("Deplacements sauvegardes !\n");
    }
}

int kbhit(){
	// la fonction retourne :
	// 1 si un caractere est present
	// 0 si pas de caractere présent
	int unCaractere=0;
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

	if(ch != EOF){
		ungetc(ch, stdin);
		unCaractere=1;
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

void enregistrerDeplacements(t_tabDeplacement t, int nb, char fic[]){
    FILE * f;

    f = fopen(fic, "w");
    fwrite(t,sizeof(char), nb, f);
    fclose(f);
}

