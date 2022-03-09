APD - Tema 1 Paralelizarea unui algoritm genetic

In primul rand, am inceput rezolvarea temei prin cautarea functiilor critice din
punct de vedere al consumului de timp. Aceastea sunt compute_fitness_function si
sortarea vectorului.

1. main
Dupa gasirea punctelor critice, a urmat paralelizarea codului. Am modificat
main-ul pentru a permite rularea pe thread-uri(adaugare variabila P pentur a
stoca numarul de thread-ur, pornirea si oprirea thread-urilor etc). Apoi am
modificat functia run_genetic_algorithm pentru a permite apelarea acesteia de
catre thread-uri: modificare tip returnat si argumente. Citind functia, se poate
observa ca sunt cateva variabile definite local care, pentru o rulare paralela,
trebuie sa fie accesabile de toate thread-urile. Astfel, mi-am creat si o
structura pentru a le trimite prin referinta, ca argumente, si le-am declarat in
functia main(structura data.h).

2. compute_fitness_function
Primul modifcare ce a trebuit facuta a fost la argumentele primite. Am adaugat
doua int-uri, start si end, pentru a delimita intervalul de lucru al thread-ului
curent, si aplicarea acestora pe for-ul exterior. Acest lucru asigura o rulare
mai rapida si nici nu modifica rezultatul functiei pentru ca modificarile ce au
loc in interiorul primului for sunt independente pentru fiecare element.

3. sortarea
Sortarea a implicat o combinatie intre MergeSort si QuickSort. Fiecare thread va
face un QuickSort pe intervalul lui. Dupa aceea, se va face un Merge pe un singur
thread pentru a asigura corectitudinea operatiilor. Functia de Merge este preluata
din implementare laboratorulului 3. Se face merge plecand de la intervalul
[0;end_thread_0] cu fiecare interval de dupa acesta, la fiecare pas marindu-se
intervalul din stanga. Dupa fiecare merge, se va copia rezultatul in vectorul
initial.

4. run_genetic_algorithm
Aceasta functie a fost adapatata pentru a putea fi apelata pe thread-uri. Se
copiaza local argumentele. Dupa aceasta, se aloca memorie pentru generatii.
Alocarea vectorilor in sine se face pe un singur thread, iar initializarea
fiecarui element din vector se fac in paralel, pe intervalul corespunzator
fiecarui thread. Dupa sincronizarea thread-urilor, in parcurgerea generatiilor,
se calculeaza fitness-ul pe fiecare interval, iar apoi acesta e sortat. Dupa
aceste operatii se face o sincronizare, se face merge la intervale si se continua
rularea operatiilor din for pe un singur thread. La finalul fiecare iteratii se
face o sincronizare. In final, se recalculeaza fitness-ul, se face sortarea pe
interval(totul in paralel), iar thread-ul 0 se asigura de merge-uitul intervalelor,
afisarea rezultatelor si eliberarea de memorie.
Precizare: sincronizarile au fost facute prin introducerea unei bariere pentru
toate thread-urile.
