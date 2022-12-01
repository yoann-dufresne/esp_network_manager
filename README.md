# Protocole d'échange

## Sur le serveur

### Première connection d'un ESP

Utilisation à l'enregistrement sur le réseau de capteur d'un nouvel ESP. Nécessite d'avoir un serveur ESP sur le réseau (classe ServerWifiManager)

Message broadcast:
```
    C
```
Le serveur répond une demande de connection directe ou transmet la demande au parent.


## Sur un noeud quelconque

### Demande de connection directe

Un ESP A enregistre un ESP B et lui transmet le message suivant pour lui demander de faire l'enregistrement inverse. C'est toujours le parent qui demande à l'enfant.

Message direct:
```
    L
```
Enregistrement mais pas de réponse attendue/


### Demande de transmission descendante

L'ESP qui envoie le message n'a pas directement accès à l'ESP qui doit recevoir le message. Il le transmet à un intermédiaire pour qu'il le retransmette.

Message direct:
```
    T D <mac addr> <msg>
```

### Demande de transmission au serveur

L'ESP envoie le message recu à son parent.

Message direct:
```
    T U <msg>
```