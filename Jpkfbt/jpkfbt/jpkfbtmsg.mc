;//--------------------------------------------------------------------
;// Definitions
;//--------------------------------------------------------------------

MessageIdTypedef=HRESULT

SeverityNames=(
  Success=0x0
  Informational=0x1
  Warning=0x2
  Error=0x3
)

FacilityNames=(
  Interface=4
)

LanguageNames=(English=0x409:MSG00409)


;//--------------------------------------------------------------------
MessageId		= 0x9300
Severity		= Warning
Facility		= Interface
SymbolicName	= STATUS_KFBT_KERNEL_NOT_SUPPORTED
Language		= English
The specified kernel is not supported by this library.
.

MessageId		= 0x9301
Severity		= Warning
Facility		= Interface
SymbolicName	= STATUS_KFBT_AGENT_NOT_FOUND
Language		= English
The FBT agent driver could not be found.
.

MessageId		= 0x9302
Severity		= Warning
Facility		= Interface
SymbolicName	= STATUS_KFBT_OPEN_SCM_FAILED
Language		= English
Contacting the SCM failed.
.

MessageId		= 0x9303
Severity		= Warning
Facility		= Interface
SymbolicName	= STATUS_KFBT_OPEN_DRIVER_FAILED
Language		= English
Opening handle to agent driver failed.
.

MessageId		= 0x9304
Severity		= Warning
Facility		= Interface
SymbolicName	= STATUS_KFBT_CREATE_DRIVER_FAILED
Language		= English
Creating handle to agent driver failed.
.

MessageId		= 0x9305
Severity		= Warning
Facility		= Interface
SymbolicName	= STATUS_KFBT_START_DRIVER_FAILED
Language		= English
Starting the agent driver failed.
.

MessageId		= 0x9306
Severity		= Warning
Facility		= Interface
SymbolicName	= STATUS_KFBT_STOP_DRIVER_FAILED
Language		= English
Stopping the agent driver failed.
.;//--------------------------------------------------------------------
;// Definitions
;//--------------------------------------------------------------------

MessageIdTypedef=HRESULT

SeverityNames=(
  Success=0x0
  Informational=0x1
  Warning=0x2
  Error=0x3
)

FacilityNames=(
  Interface=4
)

LanguageNames=(English=0x409:MSG00409)


;//--------------------------------------------------------------------
MessageId		= 0x9200
Severity		= Warning
Facility		= Interface
SymbolicName	= STATUS_FBT_NO_THUNKSTACK
Language		= English
No thunkstack available.
.

MessageId		= 0x9201
Severity		= Warning
Facility		= Interface
SymbolicName	= STATUS_FBT_PROC_NOT_PATCHABLE
Language		= English
Procedure is not patchable.
.

MessageId		= 0x9202
Severity		= Warning
Facility		= Interface
SymbolicName	= STATUS_FBT_PROC_ALREADY_PATCHED
Language		= English
Procedure already patched.
.

MessageId		= 0x9203
Severity		= Warning
Facility		= Interface
SymbolicName	= STATUS_FBT_PROC_TOO_FAR
Language		= English
The procedure address is too far away from the trampoline.
.

MessageId		= 0x9204
Severity		= Warning
Facility		= Interface
SymbolicName	= STATUS_FBT_INIT_FAILURE
Language		= English
FBT initialization failed.
.

MessageId		= 0x9205
Severity		= Warning
Facility		= Interface
SymbolicName	= STATUS_FBT_THR_SUSPEND_FAILURE
Language		= English
An error occured while trying to suspend all threads.
.

MessageId		= 0x9206
Severity		= Warning
Facility		= Interface
SymbolicName	= STATUS_FBT_THR_CTXUPD_FAILURE
Language		= English
A required instruction pointer update could not be performed.
.

MessageId		= 0x9207
Severity		= Warning
Facility		= Interface
SymbolicName	= STATUS_FBT_STILL_PATCHED
Language		= English
The library cannot be uninitialized as there are still patched procedures.
.

MessageId		= 0x9208
Severity		= Warning
Facility		= Interface
SymbolicName	= STATUS_FBT_NOT_INITIALIZED
Language		= English
Library not initialized.
.

MessageId		= 0x9209
Severity		= Warning
Facility		= Interface
SymbolicName	= STATUS_FBT_ALREADY_INITIALIZED
Language		= English
Library has already been initialized.
.

MessageId		= 0x920a
Severity		= Warning
Facility		= Interface
SymbolicName	= STATUS_FBT_UNUSABLE_TLS_SLOT
Language		= English
The TLS slot allocated cannot be used as it is an extension slot.
.

MessageId		= 0x920b
Severity		= Warning
Facility		= Interface
SymbolicName	= STATUS_FBT_AUPTR_IN_USE
Language		= English
AuxiliaryUserPointer is in use.
.

MessageId		= 0x920c
Severity		= Warning
Facility		= Interface
SymbolicName	= STATUS_FBT_NOT_PATCHED
Language		= English
Procedure not patched.
.

MessageId		= 0x920d
Severity		= Warning
Facility		= Interface
SymbolicName	= STATUS_FBT_PATCHES_ACTIVE
Language		= English
At least one patch is currently in use and cannot be removed.
.
