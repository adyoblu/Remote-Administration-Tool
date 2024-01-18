# REMOTE ADMINISTRATION TOOL
<details>
  <summary><h2>Descriere:</h2></summary>
  Aceasta aplicatie de administrare si monitorizare a sistemelor este conceputa pentru a permite administratorilor sa gestioneze si sa monitorizeze resursele sistemelor de la distanta. 
  Serverul primeste conexiuni de la clienti si ofera administratorului un set de comenzi pentru a interactiona cu sistemele conectate. 
  Aplicatia utilizeaza un model de baza client-server si include functionalitati precum afisarea informatiilor despre sistem, gestionarea proceselor, executarea de comenzi si transferul de fisiere intre client si server.
</details>

# Functionalitati:
<details>
  <summary><h2>Afisarea Informatiilor Despre Sistem:</h2></summary>
Administratorul poate solicita informatii despre sistemul unui client conectat, cum ar fi hostname-ul, lista de procese in desfasurare, informatii despre retea, versiunea sistemului de operare etc.
</details>

<details>
  <summary><h2>Executarea de Comenzi la Distanta:</h2></summary>
Administratorul are capacitatea de a executa comenzi la distanta pe sistemul client. Aceasta include orice comanda specifica sistemului de operare gazda si rezultatele executarii sunt transmise inapoi pentru afisare.
</details>

<details>
  <summary><h2>Monitorizarea Proceselor si Resurselor:</h2></summary>
Aplicatia furnizeaza informatii detaliate despre procesele care ruleaza pe sistemul client. Aceste informatii pot include utilizarea CPU, utilizarea memoriei si alte resurse relevante.
</details>

<details>
  <summary><h2>Gestionarea Conexiunilor si Deconectarea Clientilor:</h2></summary>
Administratorul poate vedea o lista de clienti conectati la server si poate decide sa deconecteze un client specific, controland astfel accesul sistemului la resursele aplicatiei.
</details>

<details>
  <summary><h2>Transfer de Fisiere:</h2></summary>
Administratorul poate trimite fisiere de la server la client sau poate primi fisiere de la un client la server. Aceasta functionalitate poate fi utila pentru distribuirea actualizarilor, configurarii sau schimbului de fisiere intre sisteme.
</details>

<details>
  <summary><h2>Blacklist/Whitelist pentru Adresele IP:</h2></summary>
Se ofera posibilitatea de a adauga adrese IP la blacklist sau whitelist.Blacklist-ul poate fi folosita pentru a bloca accesul de la anumite adrese IP, in timp ce whitelit poate limita accesul la doar adresele specificate.
Aceste functionalitati permit administratorilor sa gestioneze si sa monitorizeze sistemele conectate intr-un mod eficient si sa ia decizii in functie de informatiile obtinute si de actiunile disponibile.
</details>

<details>
  <summary><h2>Thread-uri:</h2></summary>
Thread-urile sunt utilizate pentru a gestiona concurența și pentru a permite serverului să preia conexiuni de la clienți și să furnizeze servicii în mod concurent. Thread-ul pentru meniul administratorului permite administratorului să interacționeze cu serverul în timpul execuției normale a acestuia.
</details>