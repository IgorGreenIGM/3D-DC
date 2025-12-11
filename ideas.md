-> faire une decomposiition des valuers en sous valeurs : 

Par exple, sur un ordinateur, la plus petite mémoire addessable(byte) est de 8bits sur la plupart des systèmes
pour stocker des booléens, il est en théorie nécessaire que 1 bit pour les stocker en memoire. Or, à cause de la limite inféreirue addressable en mémoie, on est obligé de les stocker sur 8bits(1 byte)
Ici notre idée d'optimisaytion consiste tout simplement en ceci : combiner les booléens et les stocker sur 1 byte.

Aimsi, au lieu de stocker un seul booléen sur 8bits, on va les combiner -en conservant l'ordre- par 8, convertir la valeur obtenue un base 256 et l'écrire sur 1 byte

exple : 

Soient : 
b1 = 1,
b2 = 0,
b3 = 0,
b4 = 1,
b5 = 1,
b6 = 1,
b7 = 0,
b8 = 0

une combinaison de ses blocs donne : b1b2b3b4b5b6b7b8 = 10011100
et dont la converszion en binaire donne : 156
on vient aisi d'économiser un total de 7 * 8 = 56 bit ~= 7 bytes


Appliquée à notre matrice de compression, il s'agira tout simplement de : 

combiner les blocs efficacement à partir de la DCMap. et les reinterpreter.
Soit la series de byte : byte1-byte2-byte3-byte4-byte5-byte6-byte7-byte8
on cherchera à faire des reinterpretations de blocs finis(de taille fixe) et à evaluer leur poids final(sur combien de bytes il peuvent s'ecrire au maximum), et si le nombre de bytes sur lesquels ils étaient écrits peuvent s'écrire sur beaucoup moins de bytes, les caster en un type connus, ce qui réduira leur taille.
Ainsi, on n'utilisera que des types de valuers fixes non signés(short(2), int(4), long long(8))