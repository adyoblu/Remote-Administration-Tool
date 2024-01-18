# REMOTE ADMINISTRATION TOOL
<details>
  <summary><h2>Descriere:</h2></summary>
  Aceasta aplicatie de administrare si monitorizare a sistemelor este conceputa pentru a permite administratorilor sa gestioneze si sa monitorizeze resursele sistemelor de la distanta. 
  Serverul primeste conexiuni de la clienti si ofera administratorului un set de comenzi pentru a interactiona cu sistemele conectate. 
  Aplicatia utilizeaza un model de baza client-server si include functionalitati precum afisarea informatiilor despre sistem, gestionarea proceselor, executarea de comenzi si transferul de fisiere intre client si server.
</details>

# Continut

- [Diagrama Block](#diagrama-block)
- [Instalare](#instalare)
- [Inainte de utilizare](#inainte-de-utilizare)
- [Utilizare](#utilizare)

# Diagrama Block
![block](https://github.com/adyoblu/Remote-Administration-Tool/assets/44545077/55d824fb-8092-4cf1-a597-b0ffcf7e7bd6)
# Instalare

Alegem un folder de pe linux, apoi deschidem un terminal si introducem comanda
```bash
git clone https://github.com/adyoblu/Remote-Administration-Tool.git
```
Apoi pentru a intra in folder
```bash
cd Remote-Administration-Tool
```

# Inainte de utilizare
### (*1*) Pentru a putea fi folosita pe 2 calculatoare diferite si a putea conecta clientii la server va trebui sa introducem in clientsocket.c adresa ip publica a server-ului la linia ```376```.
![schimbare ip server](https://iili.io/JaHsxvs.png)
### In cazul in care dorim sa conectam clientii pe acelasi calculator lasam  ```127.0.0.1```
### (*2*) Deschidem inca un terminal aditional cu combinatia ```Ctrl + Shift + N```
### (*3*) Introducem in unul din terminale comanda 
```bash
make
```

# Utilizare
### (*1*) In unul dintre terminale introducem comanda
```bash
./serversocket
```
### (*2*) In cel de-al doilea terminal deschis introducem
```bash
./clientsocket
```
### (*3*) In cazul in care avem 2 sau mai multi clienti putem repeta pasul anterior
### (*4*) Fereastra admin se deschide cand se conecteaza un client la el
![meniu admin](https://iili.io/JaHmXe9.png)
### Functionalitati optiuni
<details>
  <summary><h2>0)Iesire</h2></summary>
  <b>Aici se inchide server-ul si conexiunile clientilor.</b>
  <img src="https://iili.io/JaHyAtn.png" alt="Imagine optiunea 0">
</details>
  <b>Pentru toate optiunile de acum inainte apare o lista cu clientii conectati si va trebui sa introducem un numar 0/1/2...etc daca sunt mai multi clienti conectati.</b><br>
  <img src="https://iili.io/JaJ94m7.png" alt="Imagine optiune"><br>
<details>
  <summary><h2>1)Afiseaza hostname pentru un pc</h2></summary>
  <b>Pe server va fi afisat hostname-ul clientului caruia ii corespunde acel ip.</b>
  <img src="https://iili.io/JaJ3hV1.png" alt="hostname">
</details>
<details>
  <summary><h2>2)Afiseaza liste procese ale unui utilizator</h2></summary>
  <b>Pe server va fi afisata o lista de procese cu informatii aferente preluate din </b>
  <pre><code class="language-bash">
  /proc/[PID]/stat
  /proc/[PID]/status
  /proc/[PID]/statm
    </code>
  </pre>
  <img src="https://github.com/adyoblu/Remote-Administration-Tool/assets/44545077/5f46d9fa-2fa1-4fb3-a252-36da23b9566f" alt="procese">
</details>
<details>
  <summary><h2>3)Executa comanda pentru un utilizator</h2></summary>
  <b>Pe server va fi afisat output-ul comenzii (fara comenzi din modul super user do). Pentru comenzi fara output va fi afisat un mesaj.</b><br>
  <details>
  <summary>Exemplu comanda cu output</summary>
  <img src="https://github.com/adyoblu/Remote-Administration-Tool/assets/44545077/8b491686-7720-42eb-be61-d7da0c3b6d34" alt="comanda cu output">
  </details>
  <details>
  <summary>Exemplu comanda fara output</summary>
  <img src="https://github.com/adyoblu/Remote-Administration-Tool/assets/44545077/a40c2359-0b0b-4c5e-9194-c87a17e93bf4" alt="comanda fara output">
  </details>
</details>
<details>
  <summary><h2>4)Reporneste PC-ul unui utilizator</h2></summary>
  <b>Calculatorul clientului reporneste.</b>
</details>
<details>
  <summary><h2>5)Kick client</h2></summary>
  <b>Clientul este scos de pe server si daca este singurul conectat la server, se va sterge meniul pana la sosirea altui client.</b>
  <img src="https://github.com/adyoblu/Remote-Administration-Tool/assets/44545077/2098aacb-b692-4311-9470-4c05be72d9d9" alt="exemplu">
  <img src="https://github.com/adyoblu/Remote-Administration-Tool/assets/44545077/614e5b4a-0bf1-40f3-b93c-33de16db3c3b" alt="ex2">
</details>
<details>
  <summary><h2>6)Afiseaza toti clientii conectati</h2></summary>
  <b>Lista clientilor.</b><br>
  <img src="https://github.com/adyoblu/Remote-Administration-Tool/assets/44545077/dd533793-6f8c-47b3-9f6d-25024ff8286b" alt="ex3">
</details>
<details>
  <summary><h2>7) Blacklist/Whitelist</h2></summary>
  <details>
  <summary>Blacklist</summary>
  <img src="https://github.com/adyoblu/Remote-Administration-Tool/assets/44545077/44b81236-6d1b-4808-bb25-450d47c766f9" alt="blacklist"><br>
  <b>Apasam pe tasta "y", ne intoarcem la meniul principal, apoi tasta "5" pentru a da kick clientului de pe server. Ulterior daca clientul va dori sa intre din nou pe server va fi oprit.</b>
  <img src="https://github.com/adyoblu/Remote-Administration-Tool/assets/44545077/331206c3-b1c9-4844-9285-b24aa8644a90 alt="conexiune closed">
  </details>
  <details>
  <summary>Whitelist</summary>
  <b>In acest scenariu sunt 2 clienti conectati la server. Adaug unul dintre clienti la blacklist si ii dau kick si fiindca am un al doilea client, pot face whitelist celui caruia i-am blocat ip-ul.</b><br>
  <img src="https://github.com/adyoblu/Remote-Administration-Tool/assets/44545077/a519ab5a-9f96-498b-859a-6c5df561a379" alt="whitelist"><br>
  <img src="https://github.com/adyoblu/Remote-Administration-Tool/assets/44545077/48ef33eb-7f2a-4bbc-820b-f7db40e2d349" alt="whitelist"><br>
  </details>
</details>
<details>
  <summary><h2>8)Ia fisier de la client</h2></summary>
  <img src="https://github.com/adyoblu/Remote-Administration-Tool/assets/44545077/7ca4b6ed-1d5f-418d-b7eb-27e5aeccaa78" alt="primesc"><br>
  <img src="https://github.com/adyoblu/Remote-Administration-Tool/assets/44545077/3e96ec5c-df9d-4538-935d-413d6560aefd" alt="fisier">
</details>
<details>
  <summary><h2>9)Trimite fisier la client</h2></summary>
  <img src="https://github.com/adyoblu/Remote-Administration-Tool/assets/44545077/d9b2861a-4883-4836-b472-4508d7597275" alt="file"><br>
  <img src="https://github.com/adyoblu/Remote-Administration-Tool/assets/44545077/345d55f7-37ab-4c09-b46b-42185f17f644" alt="trimit">
</details>
