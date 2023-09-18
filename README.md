# aks
This program is a utility for extracting n-grams from texts.
It extracts every contiguous string from a collection of texts,
from length 1 up to a maximum length determined by the user.
The included scripts then perform sorting routines on the
n-gram files to determine which strings occur most frequently.
This method is especially useful for texts composed in languages
that do not feature orthographic spacing between individual
words.

usage:
./aks [language] [maximum n value] [source directory]

./processmasters [maximum n value] [source directory]

examples:

aks tibetan_roman 32 /home/handyc/texts

aks tibetan_uchen 32 /home/handyc/texts

aks chinese 32 /home/handyc/texts

aks sanskrit_unicode 32 /home/handyc/texts

You may need to change permissions on the scripts in order to allow yourself
to run them.

The best way to reach me currently is through my SDF email account.
