
# TeleSLCAN

**TeleSLCAN** est une variante du firmware **SLCAN**, conçue pour afficher et analyser les trames **CAN** dans **Teleplot**.  
Elle permet une visualisation en temps réel des données issues du bus CAN sous forme de graphique dynamique.

---

## Utilisation des commandes SLCAN avec Teleplot

### 1. Définir la vitesse du bus CAN
Dans la console de **Teleplot**, entrez la commande correspondant à la vitesse du bus :

```bash
S5\r
```

>Exemple : `S5\r` configure le bus CAN à **250 kbit/s**.

---

### 2. Ouvrir le bus CAN
Pour commencer à recevoir les trames CAN :

```O\r```

---

### 3. Fermer le bus CAN
Pour arrêter la réception des trames :

```C\r```

---

## Filtrage des trames par CAN ID

### Activer un ou plusieurs filtres
Pour ne recevoir que certaines trames spécifiques :

```F<canId>\r```

ou pour plusieurs identifiants :

```F<canId1>,<canId2>,...\r```

### Désactiver le filtrage
Pour revenir en mode normal (toutes les trames acceptées) :

```Fx\r```

---

## Tableau des vitesses CAN

| Commande  | Vitesse (kbit/s) |
|-----------|------------------|
| `S0`      | 10               |
| `S1`      | 20               |
| `S2`      | 50               |
| `S3`      | 100              |
| `S4`      | 125              |
| `S5`      | 250              |
| `S6`      | 500              |
| `S7`      | 750              |
| `S8`      | 1000             |

---

## Exemple d'utilisation dans Teleplot

1. Connectez le périphérique **TeleSLCAN** à votre ordinateur.  
2. Ouvrez **Teleplot** dans `VSCode` et sélectionnez le port série correspondant.  
3. Configurez la vitesse du bus CAN (`S5\r` pour 250 kbit/s).  
4. Activez le bus avec `O\r`.  
5. Les trames CAN apparaîtront en direct dans Teleplot.

---

## Remarque

Les messages sont formatés pour être directement interprétés par Teleplot :  
```>ID-Dx :valeur```

Exemple :

```bash

    >123-D0 :10
    >123-D1 :20
    >123-D2 :30

```

---
**Développé pour faciliter la visualisation et le débogage des trames CAN sur Teleplot.**
````