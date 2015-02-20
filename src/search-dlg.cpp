/*
 * Part of Wal Commander GitHub Edition
 * https://github.com/corporateshark/WalCommander
 * walcommander@linderdaum.com
 */

#include "ncdialogs.h"
#include "search-dlg.h"
#include "ltext.h"
#include "nceditline.h"

class SearchParamDialog: public NCVertDialog
{
	Layout iL;
public:
	StaticLabel textLabel;
	EditLine textEdit;
	SButton  caseButton;

	SearchParamDialog( NCDialogParent* parent, const SearchAndReplaceParams* params );
	virtual bool Command( int id, int subId, Win* win, void* data );
	virtual ~SearchParamDialog();
};

SearchParamDialog::~SearchParamDialog() {}

SearchParamDialog::SearchParamDialog( NCDialogParent* parent, const SearchAndReplaceParams* params )
	:  NCVertDialog( ::createDialogAsChild, 0, parent, utf8_to_unicode( _LT( "Search" ) ).data(), bListOkCancel ),
	   iL( 16, 3 ),
	   textLabel( 0, this, utf8_to_unicode( _LT( "&Search for:" ) ).data(), &textEdit ),
	   textEdit( 0, this, 0, 0, 50 ),
	   caseButton( 0, this, utf8_to_unicode( _LT( "&Case sensitive" ) ).data(), 0, params->sens )

{
	if ( params->m_SearchChar )
	{
		const unicode_t Str[] = { params->m_SearchChar, 0 };
		textEdit.SetText( Str, false );
	}
	else
	{
		if ( params->txt.data() ) { textEdit.SetText( params->txt.data(), true ); }
	}

	iL.AddWin( &textLabel, 0, 0 );
	textLabel.Enable();
	textLabel.Show();

	iL.AddWin( &textEdit, 0, 1 );
	textEdit.Enable();
	textEdit.Show();
	textEdit.SetFocus();

	iL.AddWin( &caseButton, 1, 1 );
	caseButton.Enable();
	caseButton.Show();

	AddLayout( &iL );
	SetEnterCmd( CMD_OK );

	order.append( &textEdit );
	order.append( &caseButton );

	SetPosition();
}

bool SearchParamDialog::Command( int id, int subId, Win* win, void* data )
{
	return NCVertDialog::Command( id, subId, win, data );
}

bool DoSearchDialog( NCDialogParent* parent, SearchAndReplaceParams* params )
{
	SearchAndReplaceParams LocalParams(*params);

	// always reset
	params->m_SearchChar = 0;

	SearchParamDialog dlg( parent, &LocalParams );

	if ( dlg.DoModal() == CMD_OK )
	{
		params->sens = dlg.caseButton.IsSet();
		params->txt = new_unicode_str( dlg.textEdit.GetText().data() );
		return true;
	}

	return false;
}


/////  SearchFileDialog

class SearchFileParamDialog: public NCVertDialog
{
	Layout iL;
public:
	StaticLabel maskText;
	StaticLabel textText;
	NCEditLine maskEdit;
    NCEditLine textEdit;
	SButton  caseButton;

	SearchFileParamDialog( NCDialogParent* parent, SearchAndReplaceParams* params );
	virtual ~SearchFileParamDialog();
};

SearchFileParamDialog::~SearchFileParamDialog() {}

SearchFileParamDialog::SearchFileParamDialog( NCDialogParent* parent, SearchAndReplaceParams* params )
	:  NCVertDialog( ::createDialogAsChild, 0, parent, utf8_to_unicode( _LT( "Search" ) ).data(), bListOkCancel ),
	   iL( 16, 3 ),
	   maskText( 0, this, utf8_to_unicode( _LT( "File &mask:" ) ).data(), &maskEdit ),
	   textText( 0, this, utf8_to_unicode( _LT( "&Text:" ) ).data(), &textEdit ),
       //maskEdit(0, this, 0, 0, 50),
       maskEdit( "fsearch-mask", 0, this, 0, 50, 7, false, true, false ),
       //textEdit(0, this, 0, 0, 50),
       textEdit("fsearch-text", 0, this, 0, 50, 7, false, true, false),
	   caseButton( 0, this, utf8_to_unicode( _LT( "&Case sensitive" ) ).data(), 0, params->sens )
{
	if ( params->mask.data() ) { maskEdit.SetText( params->mask.data(), true ); }

	if ( params->txt.data() ) { textEdit.SetText( params->txt.data(), true ); }

	iL.AddWin( &maskText, 0, 0 );
	maskText.Enable();
	maskText.Show();
	iL.AddWin( &textText, 1, 0 );
	textText.Enable();
	textText.Show();

	iL.AddWin( &maskEdit, 0, 1 );
	maskEdit.Enable();
	maskEdit.Show();
	maskEdit.SetFocus();

	iL.AddWin( &textEdit, 1, 1 );
	textEdit.Enable();
	textEdit.Show();

	iL.AddWin( &caseButton, 2, 1 );
	caseButton.Enable();
	caseButton.Show();

	AddLayout( &iL );
	SetEnterCmd( CMD_OK );

	order.append( &maskEdit );
	order.append( &textEdit );
	order.append( &caseButton );

	SetPosition();
}

bool DoFileSearchDialog( NCDialogParent* parent, SearchAndReplaceParams* params )
{
	SearchFileParamDialog dlg( parent, params );

	if ( dlg.DoModal() == CMD_OK )
	{
		params->sens = dlg.caseButton.IsSet();
		params->mask = new_unicode_str( dlg.maskEdit.GetText().data() );
		params->txt = new_unicode_str( dlg.textEdit.GetText().data() );
        //dlg.maskEdit.UpdateHistory();
        //dlg.textEdit.UpdateHistory();
		return true;
	}

	return false;
}


//////////////// ReplaceEdit dialog

class ReplaceEditParamDialog: public NCVertDialog
{
	Layout iL;
public:
	StaticLabel fromText;
	StaticLabel toText;
	EditLine fromEdit;
	EditLine toEdit;
	SButton  caseButton;

	ReplaceEditParamDialog( NCDialogParent* parent, SearchAndReplaceParams* params );
	virtual ~ReplaceEditParamDialog();
};

ReplaceEditParamDialog::~ReplaceEditParamDialog() {}

ReplaceEditParamDialog::ReplaceEditParamDialog( NCDialogParent* parent, SearchAndReplaceParams* params )
	:  NCVertDialog( ::createDialogAsChild, 0, parent, utf8_to_unicode( _LT( "Replace" ) ).data(), bListOkCancel ),
	   iL( 16, 3 ),
	   fromText( 0, this, utf8_to_unicode( _LT( "&Search for:" ) ).data(), &fromEdit ),
	   toText( 0, this, utf8_to_unicode( _LT( "&Replace with:" ) ).data(), &toEdit ),
	   fromEdit ( 0, this, 0, 0, 50 ),
	   toEdit   ( 0, this, 0, 0, 50 ),
	   caseButton( 0, this, utf8_to_unicode( _LT( "&Case sensitive" ) ).data(), 0, params->sens )
{
	if ( params->txt.data() ) { fromEdit.SetText( params->txt.data(), true ); }

	if ( params->to.data() ) { toEdit.SetText( params->to.data(), true ); }

	iL.AddWin( &fromText, 0, 0 );
	fromText.Enable();
	fromText.Show();
	iL.AddWin( &toText,   1, 0 );
	toText.Enable();
	toText.Show();

	iL.AddWin( &fromEdit, 0, 1 );
	fromEdit.Enable();
	fromEdit.Show();
	fromEdit.SetFocus();

	iL.AddWin( &toEdit,   1, 1 );
	toEdit.Enable();
	toEdit.Show();

	iL.AddWin( &caseButton, 2, 1 );
	caseButton.Enable();
	caseButton.Show();

	AddLayout( &iL );
	SetEnterCmd( CMD_OK );

	order.append( &fromEdit );
	order.append( &toEdit );
	order.append( &caseButton );

	SetPosition();
}

bool DoReplaceEditDialog( NCDialogParent* parent, SearchAndReplaceParams* params )
{
	ReplaceEditParamDialog dlg( parent, params );

	if ( dlg.DoModal() == CMD_OK )
	{
		params->sens = dlg.caseButton.IsSet();
		params->txt = new_unicode_str( dlg.fromEdit.GetText().data() );
		params->to = new_unicode_str( dlg.toEdit.GetText().data() );
		return true;
	}

	return false;
}



