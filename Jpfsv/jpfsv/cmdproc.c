/*----------------------------------------------------------------------
 * Purpose:
 *		Command Processor.
 *
 * Copyright:
 *		Johannes Passing (johannes.passing@googlemail.com)
 */

#include <jpfsv.h>
#include <stdlib.h>
#include <shellapi.h>
#include "internal.h"

#pragma warning( push )
#pragma warning( disable: 6011; disable: 6387 )
#include <strsafe.h>
#pragma warning( pop )


#define JPFSV_COMMAND_PROCESSOR_SIGNATURE 'PdmC'

typedef struct _JPFSV_COMMAND
{	
	union
	{
		PWSTR Name;
		JPHT_HASHTABLE_ENTRY HashtableEntry;
	} u;

	
	//
	// Routine that implements the logic.
	//
	JPFSV_COMMAND_ROUTINE Routine;

	PWSTR Documentation;
} JPFSV_COMMAND, *PJPFSV_COMMAND;

C_ASSERT( FIELD_OFFSET( JPFSV_COMMAND, u.Name ) == 
		  FIELD_OFFSET( JPFSV_COMMAND, u.HashtableEntry ) );

typedef struct _JPFSV_COMMAND_PROCESSOR
{
	DWORD Signature;
	
	//
	// Lock guarding both command execution and this data structure.
	//
	CRITICAL_SECTION Lock;

	//
	// Hashtable of JPFSV_COMMANDs.
	//
	JPHT_HASHTABLE Commands;

	//
	// State accessible by commands.
	//
	JPFSV_COMMAND_PROCESSOR_STATE State;
} JPFSV_COMMAND_PROCESSOR, *PJPFSV_COMMAND_PROCESSOR;

static BOOL JpfsvsHelp(
	__in PJPFSV_COMMAND_PROCESSOR_STATE ProcessorState,
	__in PCWSTR CommandName,
	__in UINT Argc,
	__in PCWSTR* Argv
	);

static JPFSV_COMMAND JpfsvsBuiltInCommands[] =
{
	{ { L"?" }			, JpfsvsHelp					, L"Help" },
	{ { L"echo" }		, JpfsvpEchoCommand				, L"Echo a string" },
	{ { L"|" }			, JpfsvpListProcessesCommand	, L"List processes" },
	{ { L"lm" }			, JpfsvpListModulesCommand		, L"List modules" },
	{ { L".attach" }	, JpfsvpAttachCommand			, L"Attach to current process" },
	{ { L".detach" }	, JpfsvpDetachCommand			, L"Detach from current process" },
	{ { L"tp" }			, JpfsvpSetTracepointCommand	, L"Set tracepoint" },
	{ { L"tc" }			, JpfsvpClearTracepointCommand	, L"Clear tracepoint" },
	{ { L"tl" }			, JpfsvpListTracepointsCommand	, L"List tracepoints" },
	{ { L"x" }			, JpfsvpSearchSymbolCommand		, L"Search symbol" },
	{ { L".sympath" }	, JpfsvpSymolSearchPath			, L"Manage symbol search path" },
};

/*----------------------------------------------------------------------
 * 
 * Hashtable callbacks.
 *
 */
static DWORD JpfsvsHashCommandName(
	__in DWORD_PTR Key
	)
{
	PWSTR Str = ( PWSTR ) ( PVOID ) Key;

	//
	// Hash based on djb2.
	//
	DWORD Hash = 5381;
    WCHAR Char;

    while ( ( Char = *Str++ ) != L'\0' )
	{
        Hash = ( ( Hash << 5 ) + Hash ) + Char; 
	}
    return Hash;
}


static BOOLEAN JpfsvsEqualsCommandName(
	__in DWORD_PTR KeyLhs,
	__in DWORD_PTR KeyRhs
	)
{
	PWSTR Lhs = ( PWSTR ) ( PVOID ) KeyLhs;
	PWSTR Rhs = ( PWSTR ) ( PVOID ) KeyRhs;
	
	return ( BOOLEAN ) ( 0 == wcscmp( Lhs, Rhs ) );
}

/*----------------------------------------------------------------------
 * 
 * Privates.
 *
 */

static BOOL JpfsvsHelp(
	__in PJPFSV_COMMAND_PROCESSOR_STATE ProcessorState,
	__in PCWSTR CommandName,
	__in UINT Argc,
	__in PCWSTR* Argv
	)
{
	UINT Index;
	WCHAR Buffer[ 100 ];

	UNREFERENCED_PARAMETER( ProcessorState );
	UNREFERENCED_PARAMETER( CommandName );
	UNREFERENCED_PARAMETER( Argc );
	UNREFERENCED_PARAMETER( Argv );

	for ( Index = 0; Index < _countof( JpfsvsBuiltInCommands); Index++ )
	{
		if ( SUCCEEDED( StringCchPrintf(
			Buffer,
			_countof( Buffer ),
			L"%-10s: %s\n",
			JpfsvsBuiltInCommands[ Index ].u.Name,
			JpfsvsBuiltInCommands[ Index ].Documentation ) ) )
		{
			( ProcessorState->OutputRoutine ) ( Buffer );
		}
	}

	return TRUE;
}

static HRESULT JpfsvsCreateDiagSessionAndResolver(
	__in JPFSV_OUTPUT_ROUTINE OutputRoutine,
	__out CDIAG_SESSION_HANDLE *DiagSession,
	__out PCDIAG_MESSAGE_RESOLVER *MessageResolver
	)
{
	CDIAG_SESSION_HANDLE Session = NULL;
	PCDIAG_MESSAGE_RESOLVER Resolver = NULL;
	PCDIAG_HANDLER Handler;
	HRESULT Hr;

	*DiagSession		= NULL;
	*MessageResolver	= NULL;

	Hr = CdiagCreateMessageResolver( &Resolver );
	if ( FAILED( Hr ) )
	{
		goto Cleanup;
	}

	//
	// Try to register all message DLLs that apply.
	//
	VERIFY( S_OK == Resolver->RegisterMessageDll(
		Resolver,
		L"cdiag",
		0,
		0 ) );
	VERIFY( S_OK == Resolver->RegisterMessageDll(
		Resolver,
		L"ntdll",
		0,
		0 ) );
	VERIFY( S_OK == Resolver->RegisterMessageDll(
		Resolver,
		L"jpufbt",
		0,
		0 ) );
	VERIFY( S_OK == Resolver->RegisterMessageDll(
		Resolver,
		L"jpkfbt",
		0,
		0 ) );
	VERIFY( S_OK == Resolver->RegisterMessageDll(
		Resolver,
		L"jpfsv",
		0,
		0 ) );

	Hr = CdiagCreateSession( NULL, Resolver, &Session );
	if ( FAILED( Hr ) )
	{
		goto Cleanup;
	}

	ASSERT( Session );

	Hr = CdiagCreateOutputHandler( Session, OutputRoutine, &Handler );
	if ( FAILED( Hr ) )
	{
		goto Cleanup;
	}

	ASSERT( Handler );

	Hr = CdiagSetInformationSession(
		Session,
		CdiagSessionDefaultHandler,
		0,
		Handler );
	if ( FAILED( Hr ) )
	{
		goto Cleanup;
	}

	*DiagSession		= Session;
	*MessageResolver	= Resolver;

	Hr = S_OK;

Cleanup:
	if ( FAILED( Hr ) )
	{
		if ( Session )
		{
			VERIFY( S_OK == CdiagDereferenceSession( Session ) );
		}

		if ( MessageResolver )
		{
			VERIFY( S_OK == CdiagDereferenceSession( MessageResolver ) );
		}
	}

	return Hr;
}

static JpfsvsRegisterBuiltinCommands(
	__in PJPFSV_COMMAND_PROCESSOR Processor
	)
{
	UINT Index = 0;

	//
	// Register all builtin commands.
	//
	for ( Index = 0; Index < _countof( JpfsvsBuiltInCommands ); Index++ )
	{
		PJPHT_HASHTABLE_ENTRY OldEntry;
		JphtPutEntryHashtable(
			&Processor->Commands,
			&JpfsvsBuiltInCommands[ Index ].u.HashtableEntry,
			&OldEntry );

		ASSERT( OldEntry == NULL );
	}

	ASSERT( JphtGetEntryCountHashtable( &Processor->Commands ) ==
			_countof( JpfsvsBuiltInCommands ) );
}

static JpfsvsDeleteCommandFromHashtableCallback(
	__in PJPHT_HASHTABLE Hashtable,
	__in PJPHT_HASHTABLE_ENTRY Entry,
	__in_opt PVOID Context
	)
{
	PJPHT_HASHTABLE_ENTRY OldEntry;
	
	UNREFERENCED_PARAMETER( Context );
	JphtRemoveEntryHashtable(
		Hashtable,
		Entry->Key,
		&OldEntry );

	//
	// OldEntry is from JpfsvsBuiltInCommands, so it does not be freed.
	//
}

static JpfsvsUnegisterBuiltinCommands(
	__in PJPFSV_COMMAND_PROCESSOR Processor
	)
{
	JphtEnumerateEntries(
		&Processor->Commands,
		JpfsvsDeleteCommandFromHashtableCallback,
		NULL );
}

static BOOL JpfsvsDispatchCommand(
	__in PJPFSV_COMMAND_PROCESSOR Processor,
	__in PCWSTR CommandName,
	__in UINT Argc,
	__in PWSTR* Argv
	)
{
	PJPHT_HASHTABLE_ENTRY Entry;
	Entry = JphtGetEntryHashtable(
		&Processor->Commands,
		( DWORD_PTR ) CommandName );
	if ( Entry )
	{
		PJPFSV_COMMAND CommandEntry = CONTAINING_RECORD(
			Entry,
			JPFSV_COMMAND,
			u.HashtableEntry );

		ASSERT( 0 == wcscmp( CommandEntry->u.Name, CommandName ) );

		return ( CommandEntry->Routine )(
			&Processor->State,
			CommandName,
			Argc,
			Argv );
	}
	else
	{
		JpfsvpOutput( &Processor->State, L"Unrecognized command.\n" );
		return FALSE;
	}
}

static HRESULT JpfsvsParseCommandPrefix(
	__in PCWSTR Command,
	__out PCWSTR *RemainingCommand,
	__out JPFSV_HANDLE *TempContext
	)
{
	ASSERT( TempContext );
	
	*TempContext = NULL;
	
	if ( wcslen( Command ) >= 2 && Command[ 0 ] == L'|' )
	{
		PWSTR Remain;
		DWORD Pid;
		if ( Command[ 1 ] == L'#' )
		{
			//
			// Kernel context.
			//
			HRESULT Hr = JpfsvLoadContext(
				JPFSV_KERNEL,
				NULL,
				TempContext );
			if ( SUCCEEDED( Hr ) )
			{
				*RemainingCommand = &Command[ 2 ];
				return S_OK;
			}
			else
			{
				return Hr;
			}
		}
		else if ( JpfsvpParseInteger( &Command[ 1 ], &Remain, &Pid ) )
		{
			//
			// Process context.
			//
			HRESULT Hr = JpfsvLoadContext(
				Pid,
				NULL,
				TempContext );
			if ( SUCCEEDED( Hr ) )
			{
				*RemainingCommand = Remain;
				return S_OK;
			}
			else
			{
				return Hr;
			}
		}
		else
		{
			return E_INVALIDARG;
		}
	}
	else
	{
		*RemainingCommand = Command;
		return S_OK;
	}
}

static BOOL JpfsvsParseAndDisparchCommandLine(
	__in PJPFSV_COMMAND_PROCESSOR Processor,
	__in PCWSTR CommandLine
	)
{
	INT TokenCount;
	PWSTR* Tokens;
	
	if ( JpfsvpIsWhitespaceOnly( CommandLine ) )
	{
		return FALSE;
	}

	//
	// We use CommandLineToArgvW for convenience. 
	//
	Tokens = CommandLineToArgvW( CommandLine, &TokenCount );
	if ( ! Tokens )
	{
		JpfsvpOutput( &Processor->State, L"Parsing command line failed.\n" );
		return FALSE;
	}
	else if ( TokenCount == 0 )
	{
		JpfsvpOutput( &Processor->State, L"Invalid command.\n" );
		return FALSE;
	}
	else
	{
		PWSTR RemainingCommand;
		JPFSV_HANDLE TempCtx;
		HRESULT Hr = JpfsvsParseCommandPrefix( 
			Tokens[ 0 ],
			&RemainingCommand,
			&TempCtx );
		if ( SUCCEEDED( Hr ) )
		{
			PWSTR *Argv = &Tokens[ 1 ];
			UINT Argc = TokenCount - 1;
			JPFSV_HANDLE SavedContext = NULL;
			BOOL Success = FALSE;
			if ( JpfsvpIsWhitespaceOnly( RemainingCommand ) )
			{
				//
				// First token was prefix only -> Shift.
				//
				if ( Argc > 0 )
				{
					RemainingCommand = Tokens[ 1 ];
					Argc--;
					Argv++;
				}
				else
				{
					//
					// Senseless command like '|123'.
					//
					JpfsvpOutput( &Processor->State, L"Invalid command.\n" );
					return FALSE;
				}
			}
			else if ( 0 == wcscmp( RemainingCommand, L"s" ) )
			{
				//
				// Swap contexts.
				//
				Processor->State.Context = TempCtx;
				return TRUE;
			}
			
			if ( TempCtx )
			{
				//
				// Temporarily swap contexts.
				//
				SavedContext = Processor->State.Context;
				Processor->State.Context = TempCtx;
			}

			Success = JpfsvsDispatchCommand(
				Processor,
				RemainingCommand,
				Argc,
				Argv );

			if ( TempCtx )
			{
				//
				// Restore, but do not unload context.
				//
				Processor->State.Context = SavedContext;
			}

			return Success;
		}
		else
		{
			JpfsvpOutputError( &Processor->State, Hr );
			return FALSE;
		}
	}
}

/*----------------------------------------------------------------------
 * 
 * Exports.
 *
 */

HRESULT JpfsvCreateCommandProcessor(
	__in JPFSV_OUTPUT_ROUTINE OutputRoutine,
	__in DWORD InitialProcessId, 
	__out JPFSV_HANDLE *ProcessorHandle
	)
{
	PJPFSV_COMMAND_PROCESSOR Processor = NULL;
	JPFSV_HANDLE CurrentContext = NULL;
	HRESULT Hr = E_UNEXPECTED;

	CDIAG_SESSION_HANDLE DiagSession;
	PCDIAG_MESSAGE_RESOLVER MessageResolver;

	if ( ! OutputRoutine || ! ProcessorHandle )
	{
		return E_INVALIDARG;
	}

	if ( InitialProcessId == 0 )
	{
		InitialProcessId = GetCurrentProcessId();
	}

	//
	// Create a cdiag session for output handling.
	//
	Hr = JpfsvsCreateDiagSessionAndResolver(
		OutputRoutine,
		&DiagSession,
		&MessageResolver );
	if ( FAILED( Hr ) )
	{
		return Hr;
	}

	ASSERT( MessageResolver );
	ASSERT( DiagSession );

	//
	// Create and initialize processor.
	//
	Processor = malloc( sizeof( JPFSV_COMMAND_PROCESSOR) );
	if ( ! Processor )
	{
		return E_OUTOFMEMORY;
	}

	//
	// Use context of current process by default.
	//
	Hr = JpfsvLoadContext(
		InitialProcessId,
		NULL,
		&CurrentContext );
	if ( FAILED( Hr ) )
	{
		goto Cleanup;
	}

	if ( ! JphtInitializeHashtable(
		&Processor->Commands,
		JpfsvpAllocateHashtableMemory,
		JpfsvpFreeHashtableMemory,
		JpfsvsHashCommandName,
		JpfsvsEqualsCommandName,
		_countof( JpfsvsBuiltInCommands ) * 2 - 1 ) )
	{
		Hr = E_OUTOFMEMORY;
		goto Cleanup;
	}

	Processor->Signature				= JPFSV_COMMAND_PROCESSOR_SIGNATURE;
	Processor->State.Context			= CurrentContext;
	Processor->State.DiagSession		= DiagSession;
	Processor->State.MessageResolver	= MessageResolver;
	Processor->State.OutputRoutine		= OutputRoutine;
	InitializeCriticalSection( &Processor->Lock );

	JpfsvsRegisterBuiltinCommands( Processor );

	*ProcessorHandle = Processor;
	Hr = S_OK;

Cleanup:
	if ( FAILED( Hr ) )
	{
		if ( CurrentContext )
		{
			VERIFY( SUCCEEDED( JpfsvUnloadContext( CurrentContext ) ) );
		}
		if ( Processor )
		{
			free( Processor );
		}
	}
	return Hr;
}

HRESULT JpfsvCloseCommandProcessor(
	__in JPFSV_HANDLE ProcessorHandle
	)
{
	PJPFSV_COMMAND_PROCESSOR Processor = ( PJPFSV_COMMAND_PROCESSOR ) ProcessorHandle;
	if ( ! Processor ||
		 Processor->Signature != JPFSV_COMMAND_PROCESSOR_SIGNATURE )
	{
		return E_INVALIDARG;
	}

	VERIFY( S_OK == JpfsvUnloadContext( Processor->State.Context ) );

	DeleteCriticalSection( &Processor->Lock );
	JpfsvsUnegisterBuiltinCommands( Processor );
	JphtDeleteHashtable( &Processor->Commands );

	CdiagDereferenceSession( Processor->State.DiagSession );
	Processor->State.MessageResolver->Dereference(
		Processor->State.MessageResolver );

	free( Processor );
	return S_OK;
}

JPFSV_HANDLE JpfsvGetCurrentContextCommandProcessor(
	__in JPFSV_HANDLE ProcessorHandle
	)
{
	PJPFSV_COMMAND_PROCESSOR Processor = ( PJPFSV_COMMAND_PROCESSOR ) ProcessorHandle;

	if ( Processor &&
		 Processor->Signature == JPFSV_COMMAND_PROCESSOR_SIGNATURE )
	{
		return Processor->State.Context;
	}
	else
	{
		return NULL;
	}
}

HRESULT JpfsvProcessCommand(
	__in JPFSV_HANDLE ProcessorHandle,
	__in PCWSTR CommandLine
	)
{
	PJPFSV_COMMAND_PROCESSOR Processor = ( PJPFSV_COMMAND_PROCESSOR ) ProcessorHandle;
	HRESULT Hr;
	if ( ! Processor ||
		 Processor->Signature != JPFSV_COMMAND_PROCESSOR_SIGNATURE ||
		 ! CommandLine )
	{
		return E_INVALIDARG;
	}

	EnterCriticalSection( &Processor->Lock );

	if ( JpfsvsParseAndDisparchCommandLine(
		Processor,
		CommandLine ) )
	{
		Hr = S_OK;
	}
	else
	{
		Hr = JPFSV_E_COMMAND_FAILED;
	}

	LeaveCriticalSection( &Processor->Lock );

	return Hr;
}