# M356 Interactive Computer Graphics — squelette de projet

ENSAD · filière Game Design & Animation · semestre 5

Ce dossier contient tout le nécessaire pour compiler un programme OpenGL 3.3.
**Aucune bibliothèque n'est à installer** : GLFW, GLAD, GLM et stb_image sont inclus.

## Ce qu'il faut sur la machine

| Système | À installer |
|---|---|
| Windows | Visual Studio Community, charge de travail « Développement Desktop en C++ », avec le composant « Outils CMake C++ pour Windows » |
| macOS | Les outils en ligne de commande Xcode (`xcode-select --install`) et CMake (`brew install cmake`) |

La procédure détaillée figure dans le guide d'installation déposé sur la plateforme du module.

## Compiler et lancer

### Windows, avec Visual Studio

1. Ouvrir Visual Studio, puis **Ouvrir un dossier local** et choisir ce dossier.
2. Attendre la fin de la configuration CMake, indiquée en bas de la fenêtre.
3. Vérifier que la configuration active est **x64-Debug** et la cible **app.exe**.
4. Appuyer sur **F5**.

### macOS, ou Windows en ligne de commande

```
cmake -B build
cmake --build build
./build/app
```

Sur Windows en ligne de commande, l'exécutable se trouve dans `build/Debug/app.exe`.

La première configuration compile GLFW et dure une à deux minutes. Les suivantes sont immédiates.

## Ce que vous devez voir

Une fenêtre de 1280 sur 720 pixels, un fond bleu nuit uni, la touche Échap qui ferme la fenêtre,
et dans la console deux lignes du type :

```
OpenGL  : 4.6.0 NVIDIA 550.107
Carte   : NVIDIA GeForce RTX 3050
```

Toute version supérieure ou égale à 3.3 convient.

## Contenu du dossier

```
CMakeLists.txt     description du projet
src/main.cpp       le programme, point de départ du TP 1
src/Shader.h       chargement et compilation des shaders
src/Camera.h       caméra libre, utilisée à partir du TP 4
src/Mesh.h         maillage : tampons, attributs, dessin
shaders/           base.vert et base.frag
assets/            damier.png, texture de test
extern/            glfw, glad, glm, stb : ne pas modifier
```

Les dossiers `shaders/` et `assets/` sont recopiés automatiquement à côté de l'exécutable
à chaque compilation : les chemins relatifs fonctionnent donc toujours.

## En cas de blocage

Notez le message d'erreur complet et déposez-le sur le forum du module.
Le tableau de dépannage du guide d'installation couvre les causes les plus fréquentes.
