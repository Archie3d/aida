with Ada.Wide_Text_IO.Scanning;
package body Ada.Wide_Text_IO is
    function Selected_File (Width, Which : Integer; Current : Boolean) return File_Type;
    pragma Import (C, Selected_File, "__ada_wtext_selected");
    procedure Select_File (File : File_Type; Width, Which : Integer);
    pragma Import (C, Select_File, "__ada_wtext_select");
    procedure Read_Character (File : File_Type; Item : out Wide_Character; Width, Operation : Integer; Available : out Boolean);
    pragma Import (C, Read_Character, "__ada_wtext_read_character");
    procedure Read_Text (File : File_Type; Item : out Wide_String; Width : Integer; Line : Boolean; Count : out Natural);
    pragma Import (C, Read_Text, "__ada_wtext_read_text");

    procedure Create (File : in out File_Type;
                      Mode : in File_Mode := Out_File;
                      Name : in String := "";
                      Form : in String := "") is
        procedure Native (File : in out File_Type;
                      Mode : in File_Mode := Out_File;
                      Name : in String := "";
                      Form : in String := ""; Cell_Width : Integer);
        pragma Import (C, Native, "__ada_wtext_create");
    begin
        Native (File, Mode, Name, Form, Cell_Width);
    end Create;

    procedure Open (File : in out File_Type;
                    Mode : in File_Mode;
                    Name : in String;
                    Form : in String := "") is
        procedure Native (File : in out File_Type;
                    Mode : in File_Mode;
                    Name : in String;
                    Form : in String := ""; Cell_Width : Integer);
        pragma Import (C, Native, "__ada_wtext_open");
    begin
        Native (File, Mode, Name, Form, Cell_Width);
    end Open;

    procedure Set_Input (File : in File_Type) is
    begin
        Select_File (File, Cell_Width, 0);
    end Set_Input;

    procedure Set_Output (File : in File_Type) is
    begin
        Select_File (File, Cell_Width, 1);
    end Set_Output;

    function Standard_Input return File_Type is
    begin
        return Selected_File (Cell_Width, 0, False);
    end Standard_Input;

    function Standard_Output return File_Type is
    begin
        return Selected_File (Cell_Width, 1, False);
    end Standard_Output;

    function Standard_Error return File_Type is
    begin
        return Selected_File (Cell_Width, 2, False);
    end Standard_Error;

    function Current_Input return File_Type is
    begin
        return Selected_File (Cell_Width, 0, True);
    end Current_Input;

    function Current_Output return File_Type is
    begin
        return Selected_File (Cell_Width, 1, True);
    end Current_Output;

    procedure Put_Line (Item : in Wide_String) is
    begin
        Put_Line (Current_Output, Item);
    end Put_Line;

    procedure Put (Item : in Wide_String) is
    begin
        Put (Current_Output, Item);
    end Put;

    procedure Put (Item : in Wide_Character) is
    begin
        Put (Current_Output, Item);
    end Put;

    procedure New_Line (Spacing : in Positive_Count := 1) is
    begin
        New_Line (Current_Output, Spacing);
    end New_Line;

    procedure Skip_Line (Spacing : in Positive_Count := 1) is
    begin
        Skip_Line (Current_Input, Spacing);
    end Skip_Line;

    function End_Of_File return Boolean is
    begin
        return End_Of_File (Current_Input);
    end End_Of_File;

    function End_Of_Line return Boolean is
    begin
        return End_Of_Line (Current_Input);
    end End_Of_Line;

    procedure Get (Item : out Wide_String) is
    begin
        Get (Current_Input, Item);
    end Get;

    procedure Get (Item : out Wide_Character) is
    begin
        Get (Current_Input, Item);
    end Get;

    procedure Put_Line (File : in File_Type; Item : in Wide_String) is
        procedure Native (File : in File_Type; Item : in Wide_String; Cell_Width, Operation : Integer);
        pragma Import (C, Native, "__ada_wtext_write");
    begin
        Native (File, Item, Cell_Width, 1);
    end Put_Line;

    procedure Put (File : in File_Type; Item : in Wide_String) is
        procedure Native (File : in File_Type; Item : in Wide_String; Cell_Width, Operation : Integer);
        pragma Import (C, Native, "__ada_wtext_write");
    begin
        Native (File, Item, Cell_Width, 0);
    end Put;

    procedure Put (File : in File_Type; Item : in Wide_Character) is
        procedure Native (File : in File_Type; Item : in Wide_Character; Cell_Width : Integer);
        pragma Import (C, Native, "__ada_wtext_put_character");
    begin
        Native (File, Item, Cell_Width);
    end Put;

    procedure Get (File : in File_Type; Item : out Wide_String) is
        Count : Natural;
    begin
        Read_Text (File, Item, Cell_Width, False, Count);
    end Get;

    procedure Get (File : in File_Type; Item : out Wide_Character) is
        Available : Boolean;
    begin
        Read_Character (File, Item, Cell_Width, 0, Available);
    end Get;

    function Get_Line (File : File_Type) return Wide_String is
        function Native (File : File_Type; Cell_Width : Integer) return Wide_String;
        pragma Import (C, Native, "__ada_wtext_line");
    begin
        return Native (File, Cell_Width);
    end Get_Line;

    function Get_Line return Wide_String is
    begin
        return Get_Line (Current_Input);
    end Get_Line;

    procedure Set_Error (File : File_Type) is
    begin
        Select_File (File, Cell_Width, 2);
    end Set_Error;

    function Current_Error return File_Type is
    begin
        return Selected_File (Cell_Width, 2, True);
    end Current_Error;

    procedure Flush is
    begin
        Flush (Current_Output);
    end Flush;

    procedure Set_Line_Length (To : Count) is
    begin
        Set_Line_Length (Current_Output, To);
    end Set_Line_Length;

    procedure Set_Page_Length (To : Count) is
    begin
        Set_Page_Length (Current_Output, To);
    end Set_Page_Length;

    function Line_Length return Count is
    begin
        return Line_Length (Current_Output);
    end Line_Length;

    function Page_Length return Count is
    begin
        return Page_Length (Current_Output);
    end Page_Length;

    procedure New_Page is
    begin
        New_Page (Current_Output);
    end New_Page;

    procedure Skip_Page is
    begin
        Skip_Page (Current_Input);
    end Skip_Page;

    function End_Of_Page return Boolean is
    begin
        return End_Of_Page (Current_Input);
    end End_Of_Page;

    procedure Set_Col (To : Positive_Count) is
    begin
        Set_Col (Current_Output, To);
    end Set_Col;

    procedure Set_Line (To : Positive_Count) is
    begin
        Set_Line (Current_Output, To);
    end Set_Line;

    function Col return Positive_Count is
    begin
        return Col (Current_Output);
    end Col;

    function Line return Positive_Count is
    begin
        return Line (Current_Output);
    end Line;

    function Page return Positive_Count is
    begin
        return Page (Current_Output);
    end Page;

    procedure Look_Ahead (File : File_Type ; Item : out Wide_Character; End_Of_Line : out Boolean) is
    begin
        Read_Character (File, Item, Cell_Width, 1, End_Of_Line);
    end Look_Ahead;

    procedure Look_Ahead (Item : out Wide_Character; End_Of_Line : out Boolean) is
    begin
        Look_Ahead (Current_Input, Item, End_Of_Line);
    end Look_Ahead;

    procedure Get_Immediate (File : File_Type ; Item : out Wide_Character) is
        Available : Boolean;
    begin
        Read_Character (File, Item, Cell_Width, 2, Available);
    end Get_Immediate;

    procedure Get_Immediate (Item : out Wide_Character) is
    begin
        Get_Immediate (Current_Input, Item);
    end Get_Immediate;

    procedure Get_Immediate (File : File_Type ; Item : out Wide_Character; Available : out Boolean) is
    begin
        Read_Character (File, Item, Cell_Width, 3, Available);
    end Get_Immediate;

    procedure Get_Immediate (Item : out Wide_Character; Available : out Boolean) is
    begin
        Get_Immediate (Current_Input, Item, Available);
    end Get_Immediate;

    procedure Read_Line (File : File_Type; Item : out Wide_String; Count : out Natural) is
        procedure Native (File : File_Type; Item : out Wide_String; Count : out Natural; Cell_Width : Integer);
        pragma Import (C, Native, "__ada_wtext_read_line");
    begin
        Native (File, Item, Count, Cell_Width);
    end Read_Line;
    procedure Get_Line (File : in File_Type; Item : out Wide_String; Last : out Natural) is
        count : Natural;
    begin
        Read_Line (File, Item, count);
        Last := (Item'First - 1) + count;
    end Get_Line;
    procedure Get_Line (Item : out Wide_String; Last : out Natural) is
    begin
        Get_Line (Current_Input, Item, Last);
    end Get_Line;

    package body Integer_IO is


        Digit_Set : constant Wide_String (1 .. 16) := "0123456789ABCDEF";

        -- Room for the widest thing Put can produce: a sign, a base written as two
        -- digits, the two marks around the number, and the number itself in base
        -- two.
        Text_Length : constant := 72;

        -- Item spelled out in the given base, right justified in Text and without
        -- any padding.  First says where it starts.  Ada writes a number in a base
        -- other than ten as 16#FF#, with the sign in front of the whole thing.
        procedure Image_Of (Item  : in Num;
                            Base  : in Number_Base;
                            Text  : out Wide_String;
                            First : out Natural) is
            Left     : Long_Integer := Long_Integer (Item);
            Digit    : Integer;
            Position : Natural := Text_Length + 1;
        begin
            if Base /= 10 then
                Position := Position - 1;
                Text (Position) := '#';
            end if;

            if Left = 0 then
                Position := Position - 1;
                Text (Position) := '0';
            end if;

            -- The remainder is taken from whichever side Left is on and its sign
            -- dropped afterwards, since the most negative value of a type has no
            -- positive counterpart to negate it into.
            while Left /= 0 loop
                Digit := Integer (abs (Left rem Long_Integer (Base)));
                Left := Left / Long_Integer (Base);
                Position := Position - 1;
                Text (Position) := Digit_Set (Digit + 1);
            end loop;

            -- The base itself is written the same way round as the number, so its
            -- last digit is laid down first.
            if Base /= 10 then
                Position := Position - 1;
                Text (Position) := '#';
                Position := Position - 1;
                Text (Position) := Digit_Set (Base rem 10 + 1);
                if Base >= 10 then
                    Position := Position - 1;
                    Text (Position) := Digit_Set (Base / 10 + 1);
                end if;
            end if;

            if Item < 0 then
                Position := Position - 1;
                Text (Position) := '-';
            end if;

            First := Position;
        end Image_Of;

        procedure Put (Item  : in Num;
                       Width : in Field := Default_Width;
                       Base  : in Number_Base := Default_Base) is
            Text  : Wide_String (1 .. Text_Length);
            First : Natural;
        begin
            Image_Of (Item, Base, Text, First);
            declare
                length : Natural := Text_Length - First + 1;
            begin
                if Width > length then length := Width; end if;
                declare
                    field : Wide_String (1 .. length) := (others => ' ');
                begin
                    field (length - (Text_Length - First) .. length) := Text (First .. Text_Length);
                    Scanning.Put_Field (field);
                end;
            end;
        end Put;

        procedure Put (File  : in File_Type;
                       Item  : in Num;
                       Width : in Field := Default_Width;
                       Base  : in Number_Base := Default_Base) is
            Text  : Wide_String (1 .. Text_Length);
            First : Natural;
        begin
            Image_Of (Item, Base, Text, First);
            declare
                length : Natural := Text_Length - First + 1;
            begin
                if Width > length then length := Width; end if;
                declare
                    field : Wide_String (1 .. length) := (others => ' ');
                begin
                    field (length - (Text_Length - First) .. length) := Text (First .. Text_Length);
                    Scanning.Put_Field (File, field);
                end;
            end;
        end Put;

        -- A number the instantiated type cannot hold is a Data_Error and not a
        -- Constraint_Error, which is why the range is tested here rather than left
        -- to the assignment below it.
        procedure Store (Word : in Wide_String; Item : out Num) is
            Value : Long_Integer;
        begin
            Value := Long_Integer'Wide_Value (Word);
            if Value < Long_Integer (Num'First) or else Value > Long_Integer (Num'Last) then
                raise Data_Error;
            end if;
            Item := Num (Value);
        exception
            when Constraint_Error =>
                raise Data_Error;
        end Store;

        procedure Get (Item : out Num; Width : in Field := 0) is
        begin
            Store (Scanning.Normalized (Scanning.Token (Width, 0)), Item);
        end Get;
        procedure Get (File : in File_Type; Item : out Num; Width : in Field := 0) is
        begin
            Store (Scanning.Normalized (Scanning.Token (File, Width, 0)), Item);
        end Get;
        procedure Get (From : Wide_String; Item : out Num; Last : out Positive) is
            count : Natural := Scanning.Token_Length (From, 0);
        begin
            Store (Scanning.Normalized (From (From'First .. (From'First - 1) + count)), Item);
            Last := (From'First - 1) + count;
        end Get;
        procedure Put (To : out Wide_String; Item : Num; Base : Number_Base := Default_Base) is
            text : Wide_String (1 .. Text_Length);
            first : Natural;
            length : Natural;
        begin
            Image_Of (Item, Base, text, first);
            length := Text_Length - first + 1;
            if length > To'Length then
                raise Layout_Error;
            end if;
            To := (others => ' ');
            To (To'Last - (length - 1) .. To'Last) := text (first .. Text_Length);
        end Put;
    end Integer_IO;

    package body Modular_IO is


        Digit_Set : constant Wide_String (1 .. 16) := "0123456789ABCDEF";

        -- Room for 64 binary digits, a two-digit base and both delimiters.
        Text_Length : constant := 72;

        -- Item spelled out in the given base, right justified in Text and without
        -- any padding. First says where it starts. A nondecimal base is spelled
        -- as a based literal such as 16#FF#. Modular values have no minus sign.
        procedure Image_Of (Item  : in Num;
                            Base  : in Number_Base;
                            Text  : out Wide_String;
                            First : out Natural) is
            type Wide is mod 2 ** 64;
            Left     : Wide := Wide (Item);
            Digit    : Integer;
            Position : Natural := Text_Length + 1;
        begin
            if Base /= 10 then
                Position := Position - 1;
                Text (Position) := '#';
            end if;

            if Left = 0 then
                Position := Position - 1;
                Text (Position) := '0';
            end if;

            while Left /= 0 loop
                Digit := Integer (Left rem Wide (Base));
                Left := Left / Wide (Base);
                Position := Position - 1;
                Text (Position) := Digit_Set (Digit + 1);
            end loop;

            -- The base itself is written the same way round as the number, so its
            -- last digit is laid down first.
            if Base /= 10 then
                Position := Position - 1;
                Text (Position) := '#';
                Position := Position - 1;
                Text (Position) := Digit_Set (Base rem 10 + 1);
                if Base >= 10 then
                    Position := Position - 1;
                    Text (Position) := Digit_Set (Base / 10 + 1);
                end if;
            end if;

            First := Position;
        end Image_Of;

        procedure Put (Item  : in Num;
                       Width : in Field := Default_Width;
                       Base  : in Number_Base := Default_Base) is
            Text  : Wide_String (1 .. Text_Length);
            First : Natural;
        begin
            Image_Of (Item, Base, Text, First);
            declare
                length : Natural := Text_Length - First + 1;
            begin
                if Width > length then length := Width; end if;
                declare
                    field : Wide_String (1 .. length) := (others => ' ');
                begin
                    field (length - (Text_Length - First) .. length) := Text (First .. Text_Length);
                    Scanning.Put_Field (field);
                end;
            end;
        end Put;

        procedure Put (File  : in File_Type;
                       Item  : in Num;
                       Width : in Field := Default_Width;
                       Base  : in Number_Base := Default_Base) is
            Text  : Wide_String (1 .. Text_Length);
            First : Natural;
        begin
            Image_Of (Item, Base, Text, First);
            declare
                length : Natural := Text_Length - First + 1;
            begin
                if Width > length then length := Width; end if;
                declare
                    field : Wide_String (1 .. length) := (others => ' ');
                begin
                    field (length - (Text_Length - First) .. length) := Text (First .. Text_Length);
                    Scanning.Put_Field (File, field);
                end;
            end;
        end Put;

        -- Invalid text or a value outside the instantiated range is Data_Error.
        procedure Store (Word : in Wide_String; Item : out Num) is
        begin
            Item := Num'Wide_Value (Word);
        exception
            when Constraint_Error =>
                raise Data_Error;
        end Store;

        procedure Get (Item : out Num; Width : in Field := 0) is
            Value : Ada.Wide_Text_IO.Scanning.Modular_Word;
        begin
            Value := Ada.Wide_Text_IO.Scanning.Get_Modular
                (Ada.Wide_Text_IO.Scanning.Modular_Word (Num'Base'Last), Width);
            Item := Num (Value);
        end Get;

        procedure Get (File : in File_Type; Item : out Num; Width : in Field := 0) is
            Value : Ada.Wide_Text_IO.Scanning.Modular_Word;
        begin
            Value := Ada.Wide_Text_IO.Scanning.Get_Modular
                (File, Ada.Wide_Text_IO.Scanning.Modular_Word (Num'Base'Last), Width);
            Item := Num (Value);
        end Get;

        procedure Put (To : out Wide_String; Item : in Num; Base : in Number_Base := Default_Base) is
            Text : Wide_String (1 .. Text_Length);
            First : Natural;
            Length : Natural;
        begin
            Image_Of (Item, Base, Text, First);
            Length := Text_Length - First + 1;
            if Length > To'Length then
                raise Layout_Error;
            end if;
            To := (others => ' ');
            To (To'Last - Length + 1 .. To'Last) := Text (First .. Text_Length);
        end Put;

        procedure Get (From : in Wide_String; Item : out Num; Last : out Positive) is
            Count : Natural := Ada.Wide_Text_IO.Scanning.Modular_Token (From);
        begin
            Store (From (From'First .. (From'First - 1) + Count), Item);
            Last := (From'First - 1) + Count;
        end Get;

    end Modular_IO;

    package body Float_IO is


        -- Laying a real number out is arithmetic on the digits of its
        -- representation, which is the one part of this package the C run time
        -- still does; everything about the type is decided here.
        procedure Write (Item : in Long_Float; Fore : in Field; Aft : in Field; Exp : in Field) is
            procedure Native (File : File_Type; Item : in Long_Float; Fore : in Field; Aft : in Field; Exp : in Field);
            pragma Import (C, Native, "__ada_text_put_float");
        begin
            Native (Current_Output, Item, Fore, Aft, Exp);
        end Write;

        procedure Write (File : in File_Type;
                         Item : in Long_Float;
                         Fore : in Field;
                         Aft  : in Field;
                         Exp  : in Field);
        pragma Import (C, Write, "__ada_text_put_float");

        procedure Put (Item : in Num;
                       Fore : in Field := Default_Fore;
                       Aft  : in Field := Default_Aft;
                       Exp  : in Field := Default_Exp) is
        begin
            Write (Long_Float (Item), Fore, Aft, Exp);
        end Put;

        procedure Put (File : in File_Type;
                       Item : in Num;
                       Fore : in Field := Default_Fore;
                       Aft  : in Field := Default_Aft;
                       Exp  : in Field := Default_Exp) is
        begin
            Write (File, Long_Float (Item), Fore, Aft, Exp);
        end Put;

        procedure Store (Text : Wide_String; Item : out Num) is
        begin
            Item := Num'Wide_Value (Scanning.Normalized (Text));
        exception
            when Constraint_Error => raise Data_Error;
        end Store;
        procedure Get (Item : out Num; Width : in Field := 0) is
        begin
            Store (Scanning.Token (Width, 1), Item);
        end Get;
        procedure Get (File : in File_Type; Item : out Num; Width : in Field := 0) is
        begin
            Store (Scanning.Token (File, Width, 1), Item);
        end Get;
        procedure Get (From : Wide_String; Item : out Num; Last : out Positive) is
            count : Natural := Scanning.Token_Length (From, 1);
        begin
            Store (From (From'First .. (From'First - 1) + count), Item);
            Last := (From'First - 1) + count;
        end Get;
        procedure Write (To : out Wide_String; Item : Long_Float; Aft, Exp : Field) is
            procedure Native (To : out Wide_String; Item : Long_Float; Aft, Exp : Field; Cell_Width : Integer);
            pragma Import (C, Native, "__ada_wtext_float_string");
        begin
            Native (To, Item, Aft, Exp, Cell_Width);
        end Write;
        procedure Put (To : out Wide_String; Item : Num;
                       Aft : Field := Default_Aft; Exp : Field := Default_Exp) is
        begin
            Write (To, Long_Float (Item), Aft, Exp);
        end Put;
    end Float_IO;

    package body Fixed_IO is

        -- Imported Num parameters and results carry the scaled integer directly.
        -- Small is an exact power of two, so only its exponent crosses as a float.
        function Scale (Small : Long_Float) return Integer;
        pragma Import (C, Scale, "__ada_fixed_scale");
        Bits : constant Integer := Scale (Long_Float (Num'Small));

        procedure Write (File : File_Type; Item : Num; Bits, Fore, Aft, Exp : Integer);
        pragma Import (C, Write, "__ada_fixed_put");
        procedure Write (To : out Wide_String; Item : Num; Bits, Aft, Exp : Integer) is
            procedure Native (To : out Wide_String; Item : Num; Bits, Aft, Exp : Integer; Cell_Width : Integer);
            pragma Import (C, Native, "__ada_wtext_fixed_put_string");
        begin
            Native (To, Item, Bits, Aft, Exp, Cell_Width);
        end Write;
        function Read (File : File_Type; Bits : Integer; Low, High : Num; Width : Integer) return Num;
        pragma Import (C, Read, "__ada_fixed_get");
        function Read (From : Wide_String; Bits : Integer; Low, High : Num; Last : out Natural) return Num is
            function Native (From : Wide_String; Bits : Integer; Low, High : Num; Last : out Natural; Cell_Width : Integer) return Num;
            pragma Import (C, Native, "__ada_wtext_fixed_get_string");
        begin
            return Native (From, Bits, Low, High, Last, Cell_Width);
        end Read;

        procedure Get (File : in File_Type; Item : out Num; Width : in Field := 0) is
        begin
            Item := Read (File, Bits, Num'First, Num'Last, Width);
        end Get;
        procedure Get (Item : out Num; Width : in Field := 0) is
        begin
            Get (Current_Input, Item, Width);
        end Get;
        procedure Get (From : in Wide_String; Item : out Num; Last : out Positive) is
            Count : Natural;
        begin
            Item := Read (From, Bits, Num'First, Num'Last, Count);
            Last := (From'First - 1) + Count;
        end Get;

        procedure Put (File : in File_Type; Item : in Num;
                       Fore : in Field := Default_Fore;
                       Aft : in Field := Default_Aft;
                       Exp : in Field := Default_Exp) is
        begin
            Write (File, Item, Bits, Fore, Aft, Exp);
        end Put;
        procedure Put (Item : in Num;
                       Fore : in Field := Default_Fore;
                       Aft : in Field := Default_Aft;
                       Exp : in Field := Default_Exp) is
        begin
            Put (Current_Output, Item, Fore, Aft, Exp);
        end Put;
        procedure Put (To : out Wide_String; Item : in Num;
                       Aft : in Field := Default_Aft;
                       Exp : in Field := Default_Exp) is
        begin
            Write (To, Item, Bits, Aft, Exp);
        end Put;
    end Fixed_IO;

    package body Decimal_IO is

        -- Tagged decimal scale, matching the shared fixed-point runtime ABI.
        Bits : constant Integer := 100 + Num'Scale;

        procedure Write (File : File_Type; Item : Num; Bits, Fore, Aft, Exp : Integer);
        pragma Import (C, Write, "__ada_fixed_put");
        procedure Write (To : out Wide_String; Item : Num; Bits, Aft, Exp : Integer) is
            procedure Native (To : out Wide_String; Item : Num; Bits, Aft, Exp : Integer; Cell_Width : Integer);
            pragma Import (C, Native, "__ada_wtext_fixed_put_string");
        begin
            Native (To, Item, Bits, Aft, Exp, Cell_Width);
        end Write;
        function Read (File : File_Type; Bits : Integer; Low, High : Num; Width : Integer) return Num;
        pragma Import (C, Read, "__ada_fixed_get");
        function Read (From : Wide_String; Bits : Integer; Low, High : Num; Last : out Natural) return Num is
            function Native (From : Wide_String; Bits : Integer; Low, High : Num; Last : out Natural; Cell_Width : Integer) return Num;
            pragma Import (C, Native, "__ada_wtext_fixed_get_string");
        begin
            return Native (From, Bits, Low, High, Last, Cell_Width);
        end Read;

        procedure Get (File : in File_Type; Item : out Num; Width : in Field := 0) is
        begin
            Item := Read (File, Bits, Num'First, Num'Last, Width);
        end Get;
        procedure Get (Item : out Num; Width : in Field := 0) is
        begin
            Get (Current_Input, Item, Width);
        end Get;
        procedure Get (From : in Wide_String; Item : out Num; Last : out Positive) is
            Count : Natural;
        begin
            Item := Read (From, Bits, Num'First, Num'Last, Count);
            Last := (From'First - 1) + Count;
        end Get;

        procedure Put (File : in File_Type; Item : in Num;
                       Fore : in Field := Default_Fore;
                       Aft : in Field := Default_Aft;
                       Exp : in Field := Default_Exp) is
        begin
            Write (File, Item, Bits, Fore, Aft, Exp);
        end Put;
        procedure Put (Item : in Num;
                       Fore : in Field := Default_Fore;
                       Aft : in Field := Default_Aft;
                       Exp : in Field := Default_Exp) is
        begin
            Put (Current_Output, Item, Fore, Aft, Exp);
        end Put;
        procedure Put (To : out Wide_String; Item : in Num;
                       Aft : in Field := Default_Aft;
                       Exp : in Field := Default_Exp) is
        begin
            Write (To, Item, Bits, Aft, Exp);
        end Put;
    end Decimal_IO;

    package body Enumeration_IO is
        function Lower (Item : Wide_Character) return Wide_Character;
        pragma Import (C, Lower, "__ada_wtext_lower");
        procedure Format (Item : Enum; Width : Field; Set : Type_Set; File : File_Type) is
            text : Wide_String := Enum'Wide_Image (Item);
            length : Natural := text'Length;
        begin
            if Width > length then length := Width; end if;
            for i in text'Range loop
                if Set = Lower_Case and text (text'First) /= Wide_Character'Val (39) then
                    text (i) := Lower (text (i));
                end if;
            end loop;
            declare
                field : Wide_String (1 .. length) := (others => ' ');
            begin
                field (1 .. text'Length) := text;
                Scanning.Put_Field (File, field);
            end;
        end Format;
        procedure Put (Item : Enum; Width : Field := Default_Width; Set : Type_Set := Default_Setting) is
        begin
            Format (Item, Width, Set, Current_Output);
        end Put;
        procedure Put (File : File_Type; Item : Enum;
                       Width : Field := Default_Width; Set : Type_Set := Default_Setting) is
        begin
            Format (Item, Width, Set, File);
        end Put;

        -- A word that names no literal of the type is a Data_Error here, though
        -- 'Wide_Value reports it as the Constraint_Error it is in every other setting.
        procedure Get (Item : out Enum) is

        begin
            Item := Enum'Wide_Value (Scanning.Token (0, 2));
        exception
            when Constraint_Error =>
                raise Data_Error;
        end Get;

        procedure Get (File : in File_Type; Item : out Enum) is

        begin
            Item := Enum'Wide_Value (Scanning.Token (File, 0, 2));
        exception
            when Constraint_Error =>
                raise Data_Error;
        end Get;

        procedure Get (From : Wide_String; Item : out Enum; Last : out Positive) is
            count : Natural := Scanning.Token_Length (From, 2);
        begin
            Item := Enum'Wide_Value (From (From'First .. (From'First - 1) + count));
            Last := (From'First - 1) + count;
        exception
            when Constraint_Error => raise Data_Error;
        end Get;
        procedure Put (To : out Wide_String; Item : Enum; Set : Type_Set := Default_Setting) is
            text : Wide_String := Enum'Wide_Image (Item);
        begin
            if text'Length > To'Length then
                raise Layout_Error;
            end if;
            To := (others => ' ');
            for i in text'Range loop
                if Set = Lower_Case and text (text'First) /= Wide_Character'Val (39) then
                    text (i) := Lower (text (i));
                end if;
                To (To'First + (i - text'First)) := text (i);
            end loop;
        end Put;
    end Enumeration_IO;
end Ada.Wide_Text_IO;
