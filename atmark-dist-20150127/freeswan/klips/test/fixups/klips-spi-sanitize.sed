s/\(esp0x.*\) iv=0x................ \(.*\)/\1 iv=0xABCDABCDABCDABCD \2/
s/\(life(c,s,h)=.*add(\)[0-9]*\(,[0-9]*,[0-9]*).*\)/\10\2/
s/\(life(c,s,h)=.*addtime(\)[0-9]*\(,[0-9]*,[0-9]*).*\)/\10\2/