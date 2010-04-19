# define a variable containing a single space
blank :=
space := $(blank) $(blank)
# split string $3 with $1 as separator and return the token in position $2
define token
$(word $2, $(subst $1, ,$3))
endef
# split string $3 with $1 as separator and reassemble it using the indexes in $2
define reorder
$(subst $(space),$1,$(foreach i,$2,$(call token,$1,$i,$3)))
endef
