EXTERNAL_WARNINGS_NOT_ERRORS := TRUE

PRJ=..$/..$/..$/..$/..$/..

PRJNAME=libixion
TARGET=ixionlib
ENABLE_EXCEPTIONS=TRUE
LIBTARGET=NO

.INCLUDE :  settings.mk

CFLAGS+=-I..$/..$/inc

.IF "$(GUI)"=="WNT"
CFLAGS+=-GR
.ENDIF
.IF "$(COM)"=="GCC"
CFLAGSCXX+=-frtti
.ENDIF

SLOFILES= \
			$(SLO)$/cell.obj \
			$(SLO)$/cell_queue_manager.obj \
			$(SLO)$/depends_tracker.obj \
			$(SLO)$/formula_functions.obj \
			$(SLO)$/formula_interpreter.obj \
			$(SLO)$/formula_lexer.obj \
			$(SLO)$/formula_parser.obj \
			$(SLO)$/formula_result.obj \
			$(SLO)$/formula_tokens.obj \
			$(SLO)$/global.obj \
			$(SLO)$/lexer_tokens.obj \
			$(SLO)$/mem_str_buf.obj \
			$(SLO)$/model_parser.obj \
			$(SLO)$/sort_input_parser.obj \

LIB1ARCHIV=$(LB)$/libixionlib.a
LIB1TARGET=$(SLB)$/$(TARGET).lib
LIB1OBJFILES= $(SLOFILES)

.INCLUDE :  target.mk
