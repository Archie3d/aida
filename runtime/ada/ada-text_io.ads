-- Byte text input and output. The runtime locks files and tracks text
-- positions; Ada bodies adapt line-buffer bounds and implement the nested
-- numeric and enumeration I/O generics.

with Ada.IO_Exceptions;
with Ada.IO_Handles;
with System;

package Ada.Text_IO is

    type File_Type is limited private;

    type File_Access is access all File_Type;
    type Count is range 0 .. 2147483647;
    subtype Positive_Count is Count range 1 .. Count'Last;
    Unbounded : constant Count := 0;

    type File_Mode is (In_File, Out_File, Append_File);

    -- The widths and bases the generic children below take as arguments.
    subtype Field is Integer range 0 .. 2147483647;
    subtype Number_Base is Integer range 2 .. 16;
    type Type_Set is (Lower_Case, Upper_Case);

    -- File management
    -- -----------------------------------------------------------------------

    procedure Create (File : in out File_Type;
                      Mode : in File_Mode := Out_File;
                      Name : in String := "";
                      Form : in String := "");
    pragma Import (C, Create, "__ada_create");

    procedure Open (File : in out File_Type;
                    Mode : in File_Mode;
                    Name : in String;
                    Form : in String := "");
    pragma Import (C, Open, "__ada_open");

    procedure Close (File : in out File_Type);
    pragma Import (C, Close, "__ada_file_close");

    procedure Delete (File : in out File_Type);
    pragma Import (C, Delete, "__ada_file_delete");

    procedure Reset (File : in out File_Type; Mode : in File_Mode);
    pragma Import (C, Reset, "__ada_file_reset");

    procedure Reset (File : in out File_Type);
    pragma Import (C, Reset, "__ada_file_reset_same");

    function Is_Open (File : in File_Type) return Boolean;
    pragma Import (C, Is_Open, "__ada_file_is_open");

    function Mode (File : in File_Type) return File_Mode;
    pragma Import (C, Mode, "__ada_file_mode");

    function Name (File : in File_Type) return String;
    pragma Import (C, Name, "__ada_file_name");

    function Form (File : in File_Type) return String;
    pragma Import (C, Form, "__ada_file_form");

    -- Default input and output files
    -- -----------------------------------------------------------------------

    procedure Set_Input (File : in File_Type);
    pragma Import (C, Set_Input, "__ada_set_input");

    procedure Set_Output (File : in File_Type);
    pragma Import (C, Set_Output, "__ada_set_output");

    function Standard_Input return File_Type;
    pragma Import (C, Standard_Input, "__ada_standard_input");

    function Standard_Output return File_Type;
    pragma Import (C, Standard_Output, "__ada_standard_output");

    function Standard_Error return File_Type;
    pragma Import (C, Standard_Error, "__ada_standard_error");

    function Current_Input return File_Type;
    pragma Import (C, Current_Input, "__ada_current_input");

    function Current_Output return File_Type;
    pragma Import (C, Current_Output, "__ada_current_output");

    -- Characters, strings and lines
    -- -----------------------------------------------------------------------
    -- The profiles without a file come first, so that a call naming neither a
    -- file nor anything else settles on the one working through Current_Output.

    procedure Put_Line (Item : in String);
    pragma Import (C, Put_Line, "__ada_put_line");

    procedure Put (Item : in String);
    pragma Import (C, Put, "__ada_put");

    procedure Put (Item : in Character);
    pragma Import (C, Put, "__ada_put_character");

    procedure New_Line (Spacing : in Positive_Count := 1);
    pragma Import (C, New_Line, "__ada_new_line");

    procedure Skip_Line (Spacing : in Positive_Count := 1);
    pragma Import (C, Skip_Line, "__ada_skip_line");

    function End_Of_File return Boolean;
    pragma Import (C, End_Of_File, "__ada_end_of_file");

    function End_Of_Line return Boolean;
    pragma Import (C, End_Of_Line, "__ada_end_of_line");

    procedure Get (Item : out String);
    pragma Import (C, Get, "__ada_get_string");

    procedure Get (Item : out Character);
    pragma Import (C, Get, "__ada_get");

    procedure Get_Line (Item : out String; Last : out Natural);


    procedure Put_Line (File : in File_Type; Item : in String);
    pragma Import (C, Put_Line, "__ada_text_put_line");

    procedure Put (File : in File_Type; Item : in String);
    pragma Import (C, Put, "__ada_text_put");

    procedure Put (File : in File_Type; Item : in Character);
    pragma Import (C, Put, "__ada_text_put_character");

    procedure New_Line (File : in File_Type; Spacing : in Positive_Count := 1);
    pragma Import (C, New_Line, "__ada_text_new_line");

    procedure Skip_Line (File : in File_Type; Spacing : in Positive_Count := 1);
    pragma Import (C, Skip_Line, "__ada_text_skip_line");

    function End_Of_File (File : in File_Type) return Boolean;
    pragma Import (C, End_Of_File, "__ada_file_end_of_file");

    function End_Of_Line (File : in File_Type) return Boolean;
    pragma Import (C, End_Of_Line, "__ada_text_end_of_line");

    procedure Get (File : in File_Type; Item : out String);
    pragma Import (C, Get, "__ada_text_get_string");

    procedure Get (File : in File_Type; Item : out Character);
    pragma Import (C, Get, "__ada_text_get");

    procedure Get_Line (File : in File_Type; Item : out String; Last : out Natural);


    function Get_Line (File : File_Type) return String;
    pragma Import (C, Get_Line, "__ada_text_line");
    function Get_Line return String;
    pragma Import (C, Get_Line, "__ada_line");
    procedure Set_Error (File : File_Type);
    pragma Import (C, Set_Error, "__ada_set_error");
    function Current_Error return File_Type;
    pragma Import (C, Current_Error, "__ada_current_error");
    procedure Flush (File : File_Type);
    pragma Import (C, Flush, "__ada_text_flush");
    procedure Flush;
    pragma Import (C, Flush, "__ada_flush");
    procedure Set_Line_Length (File : File_Type ; To : Count);
    pragma Import (C, Set_Line_Length, "__ada_text_set_line_length");
    procedure Set_Line_Length (To : Count);
    pragma Import (C, Set_Line_Length, "__ada_set_line_length");
    procedure Set_Page_Length (File : File_Type ; To : Count);
    pragma Import (C, Set_Page_Length, "__ada_text_set_page_length");
    procedure Set_Page_Length (To : Count);
    pragma Import (C, Set_Page_Length, "__ada_set_page_length");
    function Line_Length (File : File_Type) return Count;
    pragma Import (C, Line_Length, "__ada_text_line_length");
    function Line_Length return Count;
    pragma Import (C, Line_Length, "__ada_line_length");
    function Page_Length (File : File_Type) return Count;
    pragma Import (C, Page_Length, "__ada_text_page_length");
    function Page_Length return Count;
    pragma Import (C, Page_Length, "__ada_page_length");
    procedure New_Page (File : File_Type);
    pragma Import (C, New_Page, "__ada_text_new_page");
    procedure New_Page;
    pragma Import (C, New_Page, "__ada_new_page");
    procedure Skip_Page (File : File_Type);
    pragma Import (C, Skip_Page, "__ada_text_skip_page");
    procedure Skip_Page;
    pragma Import (C, Skip_Page, "__ada_skip_page");
    function End_Of_Page (File : File_Type) return Boolean;
    pragma Import (C, End_Of_Page, "__ada_text_end_of_page");
    function End_Of_Page return Boolean;
    pragma Import (C, End_Of_Page, "__ada_end_of_page");
    procedure Set_Col (File : File_Type ; To : Positive_Count);
    pragma Import (C, Set_Col, "__ada_text_set_col");
    procedure Set_Col (To : Positive_Count);
    pragma Import (C, Set_Col, "__ada_set_col");
    procedure Set_Line (File : File_Type ; To : Positive_Count);
    pragma Import (C, Set_Line, "__ada_text_set_line");
    procedure Set_Line (To : Positive_Count);
    pragma Import (C, Set_Line, "__ada_set_line");
    function Col (File : File_Type) return Positive_Count;
    pragma Import (C, Col, "__ada_text_col");
    function Col return Positive_Count;
    pragma Import (C, Col, "__ada_col");
    function Line (File : File_Type) return Positive_Count;
    pragma Import (C, Line, "__ada_text_line_number");
    function Line return Positive_Count;
    pragma Import (C, Line, "__ada_line_number");
    function Page (File : File_Type) return Positive_Count;
    pragma Import (C, Page, "__ada_text_page");
    function Page return Positive_Count;
    pragma Import (C, Page, "__ada_page");
    procedure Look_Ahead (File : File_Type ; Item : out Character; End_Of_Line : out Boolean);
    pragma Import (C, Look_Ahead, "__ada_text_look_ahead");
    procedure Look_Ahead (Item : out Character; End_Of_Line : out Boolean);
    pragma Import (C, Look_Ahead, "__ada_look_ahead");
    procedure Get_Immediate (File : File_Type ; Item : out Character);
    pragma Import (C, Get_Immediate, "__ada_text_get_immediate");
    procedure Get_Immediate (Item : out Character);
    pragma Import (C, Get_Immediate, "__ada_get_immediate");
    procedure Get_Immediate (File : File_Type ; Item : out Character; Available : out Boolean);
    pragma Import (C, Get_Immediate, "__ada_text_get_available");
    procedure Get_Immediate (Item : out Character; Available : out Boolean);
    pragma Import (C, Get_Immediate, "__ada_get_available");

    -- The exceptions of Ada.IO_Exceptions under the names Text_IO gives them.
    -- These are renamings, so a handler written for either one catches both.

    Status_Error : exception renames IO_Exceptions.Status_Error;
    Mode_Error   : exception renames IO_Exceptions.Mode_Error;
    Name_Error   : exception renames IO_Exceptions.Name_Error;
    Use_Error    : exception renames IO_Exceptions.Use_Error;
    Device_Error : exception renames IO_Exceptions.Device_Error;
    End_Error    : exception renames IO_Exceptions.End_Error;
    Data_Error   : exception renames IO_Exceptions.Data_Error;
    Layout_Error : exception renames IO_Exceptions.Layout_Error;

    -- Ada 2005 A.10.1: these are nested generic packages.

    generic
        type Num is range <>;

    package Integer_IO is

        -- An instance is analysed where it is written rather than where it was
        -- declared, so what it takes from its parent it names.

        Default_Width : Field := Num'Width;
        Default_Base  : Number_Base := 10;

        procedure Put (Item  : in Num;
                       Width : in Field := Default_Width;
                       Base  : in Number_Base := Default_Base);

        procedure Put (File  : in File_Type;
                       Item  : in Num;
                       Width : in Field := Default_Width;
                       Base  : in Number_Base := Default_Base);

        procedure Get (Item : out Num; Width : in Field := 0);

        procedure Get (File : in File_Type; Item : out Num; Width : in Field := 0);

        procedure Get (From : String; Item : out Num; Last : out Positive);
        procedure Put (To : out String; Item : Num; Base : Number_Base := Default_Base);
    end Integer_IO;

    generic
        type Num is mod <>;

    package Modular_IO is

        -- An instance is analysed where it is written rather than where it was
        -- declared, so what it takes from its parent it names.

        Default_Width : Field := Num'Width;
        Default_Base  : Number_Base := 10;

        procedure Put (Item  : in Num;
                       Width : in Field := Default_Width;
                       Base  : in Number_Base := Default_Base);

        procedure Put (File  : in File_Type;
                       Item  : in Num;
                       Width : in Field := Default_Width;
                       Base  : in Number_Base := Default_Base);

        procedure Get (Item : out Num; Width : in Field := 0);

        procedure Get (File : in File_Type; Item : out Num; Width : in Field := 0);

        procedure Put (To : out String; Item : in Num; Base : in Number_Base := Default_Base);
        procedure Get (From : in String; Item : out Num; Last : out Positive);

    end Modular_IO;

    generic
        type Num is digits <>;

    package Float_IO is

        -- An instance is analysed where it is written rather than where it was
        -- declared, so what it takes from its parent it names.

        Default_Fore : Field := 2;
        Default_Aft  : Field := Num'Digits - 1;
        Default_Exp  : Field := 3;

        procedure Put (Item : in Num;
                       Fore : in Field := Default_Fore;
                       Aft  : in Field := Default_Aft;
                       Exp  : in Field := Default_Exp);

        procedure Put (File : in File_Type;
                       Item : in Num;
                       Fore : in Field := Default_Fore;
                       Aft  : in Field := Default_Aft;
                       Exp  : in Field := Default_Exp);

        procedure Get (Item : out Num; Width : in Field := 0);

        procedure Get (File : in File_Type; Item : out Num; Width : in Field := 0);

        procedure Get (From : String; Item : out Num; Last : out Positive);
        procedure Put (To : out String; Item : Num; Aft : Field := Default_Aft; Exp : Field := Default_Exp);
    end Float_IO;

    generic
        type Num is delta <>;
    package Fixed_IO is

        Default_Fore : Field := Num'Fore;
        Default_Aft : Field := Num'Aft;
        Default_Exp : Field := 0;

        procedure Get (File : in File_Type; Item : out Num; Width : in Field := 0);
        procedure Get (Item : out Num; Width : in Field := 0);
        procedure Get (From : in String; Item : out Num; Last : out Positive);

        procedure Put (File : in File_Type; Item : in Num;
                       Fore : in Field := Default_Fore;
                       Aft : in Field := Default_Aft;
                       Exp : in Field := Default_Exp);
        procedure Put (Item : in Num;
                       Fore : in Field := Default_Fore;
                       Aft : in Field := Default_Aft;
                       Exp : in Field := Default_Exp);
        procedure Put (To : out String; Item : in Num;
                       Aft : in Field := Default_Aft;
                       Exp : in Field := Default_Exp);
    end Fixed_IO;

    generic
        type Num is delta <> digits <>;
    package Decimal_IO is

        Default_Fore : Field := Num'Fore;
        Default_Aft : Field := Num'Aft;
        Default_Exp : Field := 0;

        procedure Get (File : in File_Type; Item : out Num; Width : in Field := 0);
        procedure Get (Item : out Num; Width : in Field := 0);
        procedure Get (From : in String; Item : out Num; Last : out Positive);

        procedure Put (File : in File_Type; Item : in Num;
                       Fore : in Field := Default_Fore;
                       Aft : in Field := Default_Aft;
                       Exp : in Field := Default_Exp);
        procedure Put (Item : in Num;
                       Fore : in Field := Default_Fore;
                       Aft : in Field := Default_Aft;
                       Exp : in Field := Default_Exp);
        procedure Put (To : out String; Item : in Num;
                       Aft : in Field := Default_Aft;
                       Exp : in Field := Default_Exp);
    end Decimal_IO;

    generic
        type Enum is (<>);

    package Enumeration_IO is

        -- An instance is analysed where it is written rather than where it was
        -- declared, so what it takes from its parent it names.

        Default_Width   : Field := 0;
        Default_Setting : Type_Set := Upper_Case;

        procedure Put (Item  : in Enum;
                       Width : in Field := Default_Width;
                       Set   : in Type_Set := Default_Setting);

        procedure Put (File  : in File_Type;
                       Item  : in Enum;
                       Width : in Field := Default_Width;
                       Set   : in Type_Set := Default_Setting);

        procedure Get (Item : out Enum);

        procedure Get (File : in File_Type; Item : out Enum);

        procedure Get (From : String; Item : out Enum; Last : out Positive);
        procedure Put (To : out String; Item : Enum; Set : Type_Set := Default_Setting);
    end Enumeration_IO;

private

    -- The C runtime owns the file object; Ada stores only its opaque address.
    -- The wrapper starts out null so Is_Open can inspect an unopened file.
    subtype File_Handle is Ada.IO_Handles.File_Handle;

    type File_Type is
    record
        Handle : File_Handle;
    end record;

end Ada.Text_IO;
