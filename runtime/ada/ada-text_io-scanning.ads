-- Reading input in the one way that does not depend on the type being read.
--
-- Ada does not define this package; it belongs to this implementation.  The
-- generic children of Text_IO are written in Ada, and to read a value they
-- have to see where a word ends before they can say what it means.  Deciding
-- where a word ends needs to look one character past it and put that character
-- back, which the C run time can do and Ada.Text_IO cannot.  So the run time
-- hands over the word, and the generic that asked for it decides what it is.

with Ada.Text_IO;

package Ada.Text_IO.Scanning is

    procedure Put_Field (File : File_Type; Text : String);
    pragma Import (C, Put_Field, "__ada_text_put_field");
    procedure Put_Field (Text : String);
    pragma Import (C, Put_Field, "__ada_put_field");

    -- Kind 0: integer, 1: real, 2: enumeration. File tokens have no size cap.
    function Token (File : File_Type; Width : Field; Kind : Integer) return String;
    pragma Import (C, Token, "__ada_text_scan");
    function Token (Width : Field; Kind : Integer) return String;
    pragma Import (C, Token, "__ada_scan");
    function Token_Length (Text : String; Kind : Integer) return Natural;
    pragma Import (C, Token_Length, "__ada_io_token_length");
    function Normalized (Text : String) return String;

    -- The next run of non blank characters, with the blanks before it skipped.
    -- Raises End_Error if the input has nothing left.
    procedure Get_Word (Item : out String; Last : out Natural);
    pragma Import (C, Get_Word, "__ada_get_word");

    procedure Get_Word (File : in File_Type; Item : out String; Last : out Natural);
    pragma Import (C, Get_Word, "__ada_text_get_word");

    -- The real number a word spells out.  Raises Data_Error if it spells none,
    -- which is what Float_IO.Get reports when the input is not a number.
    function Real_Value (Text : in String) return Long_Float;
    pragma Import (C, Real_Value, "__ada_real_value");

    type Modular_Word is mod 2 ** 64;
    function Get_Modular (Last : Modular_Word; Width : Field) return Modular_Word;
    pragma Import (C, Get_Modular, "__ada_get_modular");
    function Get_Modular (File : File_Type; Last : Modular_Word; Width : Field) return Modular_Word;
    pragma Import (C, Get_Modular, "__ada_text_get_modular");
    function Modular_Token (Text : String) return Natural;
    pragma Import (C, Modular_Token, "__ada_modular_token");

end Ada.Text_IO.Scanning;
