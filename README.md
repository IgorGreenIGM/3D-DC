# Algorithme de Compression de Données 3D

Ce projet présente un algorithme de compression de données 3D développé en C++. L'objectif principal de cet algorithme est de réduire la taille des fichiers en se basant sur le principe de la **réduction de l'entropie de Shannon**. En appliquant des méthodes de filtrage prédictif, l'algorithme diminue la redondance de l'information, ce qui rend les données plus aptes à la compression.

Ce référentiel contient non seulement le code source de l'algorithme, mais aussi des scripts Python pour effectuer des tests de performance et visualiser les résultats.

## Principe Théorique

La compression de données repose sur l'élimination de la redondance. L'**entropie de Shannon** est une mesure de l'incertitude ou du désordre dans un ensemble de données. Plus l'entropie est élevée, plus les données sont aléatoires et difficiles à compresser.

Cet algorithme utilise des filtres prédictifs pour estimer la valeur d'un point de données en fonction de ses voisins. En stockant uniquement la différence (ou résidu) entre la valeur prédite et la valeur réelle, l'algorithme réduit la variance des données. Cet ensemble de résidus a une entropie plus faible que les données originales, ce qui permet une compression plus efficace par des algorithmes standards comme Huffman ou Lempel-Ziv.

## Architecture des Modules

L'algorithme est structuré en plusieurs modules, chacun ayant un rôle spécifique dans le processus de compression.

### 1. `DCBuffer`

Le module `DCBuffer` est le point d'entrée du flux de données. Il lit les données depuis un fichier ou une zone mémoire et les segmente en blocs (chunks) de taille fixe. Cette approche permet de traiter des fichiers volumineux sans avoir à les charger intégralement en mémoire.

-   **Entrée** : Chemin d'un fichier ou pointeur mémoire.
-   **Sortie** : Blocs de données prêts à être traités par le module suivant.

### 2. `DCMatrix`

`DCMatrix` est une structure de données qui représente une matrice carrée. Elle sert d'unité de base pour organiser les blocs de données reçus de `DCBuffer`.

### 3. `DCQueue`

`DCQueue` est le cœur de l'algorithme de compression. Il organise les `DCMatrix` en une file d'attente et applique des filtres prédictifs pour réduire l'entropie.

-   **Filtres 2D et 3D** : Des filtres prédictifs (comme `Sub`, `Up`, `Average`, `Paeth`) sont appliqués en deux et trois dimensions. Chaque filtre prédit la valeur d'un point de données en se basant sur ses voisins. Le résidu est ensuite stocké.
-   **Réduction d'entropie** : En remplaçant les valeurs originales par des résidus, la distribution des données devient moins aléatoire, ce qui diminue l'entropie globale.

### 4. `DCMap`

Le module `DCMap` ajoute une couche d'abstraction en superposant un système de coordonnées 3D sur la `DCQueue`. Cela permet de manipuler l'ensemble de données comme un volume 3D, facilitant des opérations complexes comme :

-   L'analyse de points de données voisins dans un espace tridimensionnel.
-   La navigation et l'extraction de sous-ensembles de données.

## Guide de Démarrage

### Prérequis

-   Un compilateur C++ (par exemple, GCC).
-   Python 3.x.
-   Les bibliothèques Python `matplotlib`.
-   L'outil de compression `rar` (pour les scripts de test).

### Compilation

Pour compiler le programme C++, exécutez la commande suivante à la racine du projet :

```bash
make
```

Cette commande générera un fichier exécutable nommé `output.exe`.

### Utilisation

Le script `scrapper.py` est conçu pour tester l'algorithme avec différents paramètres. Il exécute le programme C++ sur une image (`img.png`) en faisant varier la taille des matrices et en appliquant les différents filtres (2D, 3D et combinés).

```bash
python scrapper.py
```

Les résultats des tests sont enregistrés dans les fichiers `matrix_2D.txt`, `matrix_3D.txt`, et `matrix_2D_3D.txt`.

### Visualisation

Le notebook Jupyter `plotter.ipynb` permet de visualiser les performances de compression. Il lit les fichiers de résultats et génère un graphique comparant la taille du fichier original à celle des fichiers compressés après l'application des différents filtres. Cela permet d'analyser l'efficacité de chaque méthode de filtrage.
