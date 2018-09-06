#!/cygdrive/c/Python27/Python.exe

pinList = \
(\
 ("Encoder", "out", "encoder pulse output"),
 ("SyncOut", "out", "sync signal output"),
 ("Led", "out", "led output"),
 ("A", "in", "a encoder input"),
 ("B", "in", "b encoder input"),
 ("ATest", "out", "a test signal"),
 ("BTest", "out", "b test signal"),
 ("IndexTest", "out", "sync test signal"),
)

dbgList = \
(\
 ("dbgIntC",    0, "toggle on capture input"),
 ("dbgIntP",    1, "toggle on output pulse"),
 ("dbgCycle",   2, "toggle on input cycle"),
 ("dbgCycEnd",  3, "end of internal pulse cycle"),
 ("dbgCapIsr",  4, "length of capture isr"),
 ("dbgIntIsr",  5, "length of internal isr"),
)

dbgPins = (0, 1, 2, 3, 4, 5)

trackSize = 64

dbgTrk = "2WL0"

dbgTrkList = \
(\
 ("2WL0", "encoder capature"),
 ("2WL1", "interal capature"),
)

f = open("include/pins.h", "wb")

for (name, func, comment) in pinList:
    f.write("/* %s */\n" % (comment))
    var = name[0].lower() + name[1:]
    if func == "out":
        f.write("#define %sSet() %s_GPIO_Port->BSRR = %s_Pin\n" % \
                (var, name, name))
        f.write("#define %sClr() %s_GPIO_Port->BSRR = (%s_Pin << 16)\n" % \
                (var, name, name))
        f.write("#define %s() ((%s_Pin & %s_GPIO_Port->ODR) != 0)\n" % \
                (var, name, name))
    elif func == "in":
        f.write("#define %s() ((%s_Pin & %s_GPIO_Port->IDR) != 0)\n" % \
                (var, name, name))
    f.write("\n")

f.close()

f = open("include/dbg.h", "wb")
f1 = open("include/dbgPin.h", "wb")

for pin in dbgPins:
    f.write("#define DBG%d 1\n" % (pin))
f.write("\n")
            
for pin in dbgPins:
    f.write("#ifdef Dbg%d_Pin\n" % (pin))
    f.write("#define dbg%dIni()\n" % (pin))
    f.write("#define dbg%dSet() Dbg%d_GPIO_Port->BSRR = Dbg%d_Pin\n" % \
            (pin, pin, pin))
    f.write("#define dbg%dClr() Dbg%d_GPIO_Port->BSRR = (Dbg%d_Pin << 16)\n" %\
            (pin, pin, pin))
    f.write("#else\n")
    f.write("#define dbg%dIni()\n" % (pin))
    f.write("#define dbg%dSet()\n" % (pin))
    f.write("#define dbg%dClr()\n" % (pin))
    f.write("#endif\n\n")

for (name, pin, comment) in dbgList:
    pin = int(pin)
    f.write("/* %s */\n" % (comment))
    if pin >= 0:
        f.write("#ifdef Dbg%d_Pin\n" % (pin))
        f.write("#define %sSet() Dbg%d_GPIO_Port->BSRR = Dbg%d_Pin\n" % \
                (name, pin, pin))
        f.write("#define %sClr() "
                "Dbg%d_GPIO_Port->BSRR = (Dbg%d_Pin << 16)\n" % \
                (name, pin, pin))
        f.write("#else\n")
        f.write("#define %sSet()\n" % (name))
        f.write("#define %sClr()\n" % (name))
        f.write("#endif\n\n")

        f1.write("#ifdef Dbg%d_Pin\n" % (pin))
        f1.write(" PIN(%s, Dbg%d),\n" % (name, pin))
        f1.write("#endif\n")
    else:
        f.write("#define %sSet()\n" % (name))
        f.write("#define %sClr()\n\n" % (name))
f.close()
f1.close()

dbgTrkCode = \
(\
 ("1W", "val1", \
  "{\\\n" \
  " int16_t *p = (int16_t *) &trkbuf[trkidx];\\\n" \
  " *p = val1;\\\n" \
  " trkidx += 1;\\\n" \
  " trkidx &= (TRKBUFSIZE - 1);\\\n" \
  "}\n"), \

 ("1L", "val1", \
  "{\\\n" \
  " int32_t *p = (int32_t *) &trkbuf[trkidx];\\\n" \
  " *p = val1;\\\n" \
  " trkidx += 2;\\\n" \
  " trkidx &= (TRKBUFSIZE - 1);\\\n" \
  "}\n"), \

 ("2L", "val1, val2",
  "{\\\n" \
  " int32_t *p = (int32_t *) &trkbuf[trkidx];\\\n" \
  " *p++ = val1;\\\n" \
  " *p = val2;\\\n" \
  " trkidx += 4;\\\n" \
  " trkidx &= (TRKBUFSIZE - 1);\\\n" \
  "}\n"), \

 ("2WL", "val1, val2, val3",
  "{\\\n" \
  " if (dbgTrk)\\\n" \
  " {\\\n" \
  "  int16_t *p = (int16_t *) &trkbuf[trkidx];\\\n" \
  "  *p++ = val1;\\\n" \
  "  *p++ = val2;\\\n" \
  "  *((int32_t *) p) = val3;\\\n" \
  "  trkidx += 4;\\\n" \
  "  trkidx &= (TRKBUFSIZE - 1);\\\n" \
  " }\\\n" \
  "}\n"), \
)

f = open("include/dbgtrk.h", "wb")

code = dbgTrk[:-1]

f.write("#define DBGTRK 1\n\n")

f.write("#if DBGTRK\n\n")

f.write("EXT boolean dbgTrk;\n")
f.write("#define TRKBUFSIZE (4*%d)\n" % trackSize)
f.write("EXT int16_t trkidx;\n")
f.write("EXT int16_t trkbuf[TRKBUFSIZE];\n\n")

for (label, comment) in dbgTrkList:
    define = "#define DBGTRK%s %d" % (label, label == dbgTrk)
    f.write("%s/* %s */\n" % (define.ljust(32), comment))

f.write("\n")

for (label, args, macro) in dbgTrkCode:
    defined = label == code
    f.write("#define DBGTRK%s %d\n" % (label, defined))
    f.write("#define dbgTrk%s(%s)" % (label, args))
    if defined:
        f.write(" \\\n%s\n" % ( macro))
    else:
        f.write("\n\n")
                
f.write("#endif /* DBGTRK */\n")
f.close()
