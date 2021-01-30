(cd tests && sleep 1 && wc -l sample.txt)&
(echo one && echo three) | (false || (echo two ; tac))
((((sleep 3&&echo \
    	"    ;4)"););););echo 4
