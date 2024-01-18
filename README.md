# REMOTE ADMINISTRATION TOOL
<details>
  <summary>
# Descriere:
Aceasta aplicatie de administrare si monitorizare a sistemelor este conceputa pentru a permite administratorilor sa gestioneze si sa monitorizeze resursele sistemelor de la distanta. 
Serverul primeste conexiuni de la clienti si ofera administratorului un set de comenzi pentru a interactiona cu sistemele conectate. 
Aplicatia utilizeaza un model de baza client-server si include functionalitati precum afisarea informatiilor despre sistem, gestionarea proceselor, executarea de comenzi si transferul de fisiere intre client si server.
 </summary>
</details>

## Functionalitati:
<details>
  <summary>

### Afisarea Informatiilor Despre Sistem:
Administratorul poate solicita informatii despre sistemul unui client conectat, cum ar fi hostname-ul, lista de procese in desfasurare, informatii despre retea, versiunea sistemului de operare etc.

### Executarea de Comenzi la Distanta:
Administratorul are capacitatea de a executa comenzi la distanta pe sistemul client. Aceasta include orice comanda specifica sistemului de operare gazda si rezultatele executarii sunt transmise inapoi pentru afisare.

### Monitorizarea Proceselor si Resurselor:
Aplicatia furnizeaza informatii detaliate despre procesele care ruleaza pe sistemul client. Aceste informatii pot include utilizarea CPU, utilizarea memoriei si alte resurse relevante.

### Gestionarea Conexiunilor si Deconectarea Clientilor:
Administratorul poate vedea o lista de clienti conectati la server si poate decide sa deconecteze un client specific, controland astfel accesul sistemului la resursele aplicatiei.

### Transfer de Fisiere:
Administratorul poate trimite fisiere de la server la client sau poate primi fisiere de la un client la server. Aceasta functionalitate poate fi utila pentru distribuirea actualizarilor, configurarii sau schimbului de fisiere intre sisteme.

### Blacklist/Whitelist pentru Adresele IP:
Se ofera posibilitatea de a adauga adrese IP la blacklist sau whitelist.Blacklist-ul poate fi folosita pentru a bloca accesul de la anumite adrese IP, in timp ce whitelit poate limita accesul la doar adresele specificate.
Aceste functionalitati permit administratorilor sa gestioneze si sa monitorizeze sistemele conectate intr-un mod eficient si sa ia decizii in functie de informatiile obtinute si de actiunile disponibile.

### Thread-uri:
Thread-urile sunt utilizate pentru a gestiona concurența și pentru a permite serverului să preia conexiuni de la clienți și să furnizeze servicii în mod concurent. Thread-ul pentru meniul administratorului permite administratorului să interacționeze cu serverul în timpul execuției normale a acestuia.
</summary>
</details>