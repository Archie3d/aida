with Ada.Text_IO; use Ada.Text_IO;
with Ada.Storage_IO;
with Ada.IO_Exceptions;
with Ada.Finalization;
with Ada.Unchecked_Deallocation;
with Address_Views;
with System;
with System.Storage_Elements; use System.Storage_Elements;
procedure StorageIO is
    package Integers is new Ada.Storage_IO (Integer);
    package Again is new Ada.Storage_IO (Integer);
    procedure Save (Buffer : out Integers.Buffer_Type; Item : Integer) renames Integers.Write;
    type Reader is access procedure (Buffer : in Integers.Buffer_Type; Item : out Integer);
    Load : Reader := Integers.Read'Access;
    generic
        type Item is private;
    procedure Generic_Check (Original : Item);
    procedure Generic_Check (Original : Item) is
        package IO is new Ada.Storage_IO (Item);
        B : IO.Buffer_Type;
        Copy : Item := Original;
    begin
        IO.Write (B, Original);
        IO.Read (B, Copy);
        if Copy /= Original then raise Program_Error; end if;
    end Generic_Check;
    procedure Generic_Integer is new Generic_Check (Integer);
    procedure Check (Value : Boolean) is
    begin
        if not Value then raise Program_Error; end if;
    end Check;
    procedure Run is
        B : Integers.Buffer_Type;
        X : Integer := 0;
        Calls : Natural := 0;
        function Value return Integer is
        begin
            Calls := Calls + 1;
            return -123456;
        end Value;
        function Index return Storage_Offset is
        begin
            Calls := Calls + 1;
            return 2;
        end Index;
        Backing : Storage_Array (0 .. Integers.Buffer_Size + 3) := (others => 85);
        Short : Storage_Array (1 .. Integers.Buffer_Size - 1);
        Failed : Boolean := False;
        subtype Small is Integer range -3 .. 7;
        package Small_IO is new Ada.Storage_IO (Small);
        Small_Buffer : Small_IO.Buffer_Type;
        Small_Value : Small := 3;
        type Nibble is range 0 .. 7;
        for Nibble'Size use 8;
        package Nibbles is new Ada.Storage_IO (Nibble);
        Nibble_Buffer : Nibbles.Buffer_Type;
        N : Nibble := 0;
        type Compact is record
            Flag : Boolean;
            Code : Nibble;
        end record;
        for Compact use record
            Flag at 0 range 0 .. 0;
            Code at 0 range 1 .. 3;
        end record;
        for Compact'Size use 8;
        package Compacts is new Ada.Storage_IO (Compact);
        Compact_Buffer : Compacts.Buffer_Type;
        Packed : Compact := (True, 5);
        type Fixed_Value is delta 0.25 range -8.0 .. 7.75;
        package Fixed_IO is new Ada.Storage_IO (Fixed_Value);
        Fixed_Buffer : Fixed_IO.Buffer_Type;
        Fixed : Fixed_Value := 0.0;
        type Wide_Mod is mod 2 ** 64;
        package Mods is new Ada.Storage_IO (Wide_Mod);
        Mod_Buffer : Mods.Buffer_Type;
        Unsigned : Wide_Mod := 0;
        type Color is (Red, Green, Blue);
        for Color use (-11, 27, 92);
        package Colors is new Ada.Storage_IO (Color);
        Color_Buffer : Colors.Buffer_Type;
        C : Color := Red;
        subtype Text is String (3 .. 7);
        package Texts is new Ada.Storage_IO (Text);
        Text_Buffer : Texts.Buffer_Type;
        S : Text := "abcde";
        type Matrix is array (2 .. 3, -1 .. 1) of Integer;
        package Matrices is new Ada.Storage_IO (Matrix);
        Matrix_Buffer : Matrices.Buffer_Type;
        M : Matrix := ((1, 2, 3), (4, 5, 6));
        type Bits is array (1 .. 9) of Boolean;
        pragma Pack (Bits);
        package Bit_IO is new Ada.Storage_IO (Bits);
        Bit_Buffer : Bit_IO.Buffer_Type;
        Flags : Bits := (1 | 9 => True, others => False);
        type Empty is array (1 .. 0) of Integer;
        package Empty_IO is new Ada.Storage_IO (Empty);
        Empty_Buffer : Empty_IO.Buffer_Type;
        Nothing : Empty;
        type Padded is record
            First : Storage_Element;
            Last : Integer;
        end record;
        package Records is new Ada.Storage_IO (Padded);
        Record_Buffer : Records.Buffer_Type;
        R : Padded := (7, 123);
        type Small_Record is record
            First : Storage_Element;
            Last : Small;
        end record;
        package Small_Records is new Ada.Storage_IO (Small_Record);
        Bad_Record : Small_Records.Buffer_Type;
        SR : Small_Record := (9, 4);
        package Private_IO is new Ada.Storage_IO (Address_Views.Hidden);
        Private_Buffer : Private_IO.Buffer_Type;
        Hidden : Address_Views.Hidden := Address_Views.Create (42);
        type Ref is access Integer;
        procedure Free is new Ada.Unchecked_Deallocation (Integer, Ref);
        package Refs is new Ada.Storage_IO (Ref);
        Ref_Buffer : Refs.Buffer_Type;
        Owner : Ref := new Integer'(71);
        Borrowed : Ref;
        type Variant (Choice : Boolean := False) is record
            case Choice is
                when False => Number : Integer;
                when True => Letter : Character;
            end case;
        end record;
        package Variants is new Ada.Storage_IO (Variant);
        Variant_Buffer : Variants.Buffer_Type;
        V : Variant := (Choice => True, Letter => 'Z');
        Live : Integer := 0;
        Adjustments : Natural := 0;
        Resources : Natural := 0;
        Fail_Adjust : Boolean := False;
        type Resource is record
            References : Natural;
        end record;
        type Resource_Ref is access Resource;
        procedure Release is new Ada.Unchecked_Deallocation (Resource, Resource_Ref);
        type Guard is new Ada.Finalization.Controlled with record
            Payload : Integer := 0;
            Data : Resource_Ref;
        end record;
        overriding procedure Initialize (Object : in out Guard);
        overriding procedure Adjust (Object : in out Guard);
        overriding procedure Finalize (Object : in out Guard);
        overriding procedure Initialize (Object : in out Guard) is
        begin
            Live := Live + 1;
            Object.Data := new Resource'(References => 1);
            Resources := Resources + 1;
        end Initialize;
        overriding procedure Adjust (Object : in out Guard) is
        begin
            if Fail_Adjust then raise Constraint_Error; end if;
            Live := Live + 1;
            Adjustments := Adjustments + 1;
            Check (Object.Data /= null and then Object.Data.References > 0);
            Object.Data.References := Object.Data.References + 1;
        end Adjust;
        overriding procedure Finalize (Object : in out Guard) is
        begin
            Live := Live - 1;
            Check (Object.Data /= null and then Object.Data.References > 0);
            Object.Data.References := Object.Data.References - 1;
            if Object.Data.References = 0 then
                Resources := Resources - 1;
                Release (Object.Data);
            end if;
        end Finalize;
        package Guards is new Ada.Storage_IO (Guard);
        Guard_Buffer : Guards.Buffer_Type;
    begin
        Save (Item => Value, Buffer => Backing (Index .. Integers.Buffer_Size + 1));
        Check (Calls = 2 and Backing (0) = 85 and Backing (1) = 85
            and Backing (Integers.Buffer_Size + 2) = 85);
        Again.Read (Backing (2 .. Integers.Buffer_Size + 1), X);
        Check (X = -123456 and Integers.Buffer_Size = Integer'Max_Size_In_Storage_Elements);
        B := Backing (2 .. Integers.Buffer_Size + 1);
        Load (B, X);
        Check (X = -123456);
        Generic_Integer (X);
        begin
            Integers.Read (Short, X);
        exception
            when Constraint_Error => Failed := True;
        end;
        Check (Failed);
        Integers.Write (Small_Buffer, 8);
        Failed := False;
        begin
            Small_IO.Read (Small_Buffer, Small_Value);
        exception
            when Ada.IO_Exceptions.Data_Error => Failed := True;
        end;
        Check (Failed and Small_Value = 3);
        Nibbles.Write (Nibble_Buffer, 5);
        Nibbles.Read (Nibble_Buffer, N);
        Check (N = 5 and Nibbles.Buffer_Size = 1);
        Compacts.Write (Compact_Buffer, Packed);
        Check (Compacts.Buffer_Size = 1 and Compact_Buffer (1) = 11);
        Packed := (False, 0);
        Compacts.Read (Compact_Buffer, Packed);
        Check (Packed.Flag and Packed.Code = 5);
        Fixed_IO.Write (Fixed_Buffer, -3.25);
        Fixed_IO.Read (Fixed_Buffer, Fixed);
        Check (Fixed = -3.25);
        Mods.Write (Mod_Buffer, Wide_Mod'Last);
        Mods.Read (Mod_Buffer, Unsigned);
        Check (Unsigned = Wide_Mod'Last);
        Colors.Write (Color_Buffer, Blue);
        Colors.Read (Color_Buffer, C);
        Check (C = Blue);
        Color_Buffer := (others => 255);
        Failed := False;
        begin
            Colors.Read (Color_Buffer, C);
        exception
            when Colors.Data_Error => Failed := True;
        end;
        Check (Failed and C = Blue);
        Texts.Write (Text_Buffer, S);
        S := "xxxxx";
        Texts.Read (Text_Buffer, S);
        Check (S = "abcde");
        Matrices.Write (Matrix_Buffer, M);
        M := (others => (others => 0));
        Matrices.Read (Matrix_Buffer, M);
        Check (M (2, -1) = 1 and M (3, 1) = 6);
        Bit_IO.Write (Bit_Buffer, Flags);
        Check (Bit_IO.Buffer_Size = 2 and Bit_Buffer (2) = 1);
        Flags := (others => False);
        Bit_IO.Read (Bit_Buffer, Flags);
        Check (Flags (1) and Flags (9) and not Flags (8));
        Empty_IO.Write (Empty_Buffer, Nothing);
        Empty_IO.Read (Empty_Buffer, Nothing);
        Check (Empty_IO.Buffer_Size = 0 and Empty_Buffer'Length = 0);
        Records.Write (Record_Buffer, R);
        Check (Record_Buffer (1) = 7 and Record_Buffer (2) = 0
            and Record_Buffer (3) = 0 and Record_Buffer (4) = 0);
        R := (0, 0);
        Records.Read (Record_Buffer, R);
        Check (R.First = 7 and R.Last = 123);
        Bad_Record := Record_Buffer;
        Failed := False;
        begin
            Small_Records.Read (Bad_Record, SR);
        exception
            when Small_Records.Data_Error => Failed := True;
        end;
        Check (Failed and SR.First = 9 and SR.Last = 4);
        Private_IO.Write (Private_Buffer, Hidden);
        Hidden := Address_Views.Create (0);
        Private_IO.Read (Private_Buffer, Hidden);
        Check (Address_Views.Value (Hidden) = 42);
        Refs.Write (Ref_Buffer, Owner);
        Refs.Read (Ref_Buffer, Borrowed);
        Check (Borrowed = Owner and Borrowed.all = 71);
        Borrowed := null;
        Free (Owner);
        Refs.Write (Ref_Buffer, Owner);
        Refs.Read (Ref_Buffer, Borrowed);
        Check (Borrowed = null);
        Variants.Write (Variant_Buffer, V);
        V := (Choice => False, Number => 0);
        Variants.Read (Variant_Buffer, V);
        Check (V.Choice and V.Letter = 'Z');
        declare
            Original, Copy : Guard;
            Before : Natural;
        begin
            Original.Payload := 19;
            Check (Live = 2);
            Before := Adjustments;
            Guards.Write (Guard_Buffer, Original);
            Check (Live = 2 and Adjustments = Before);
            Guards.Read (Guard_Buffer, Copy);
            Check (Copy.Payload = 19 and Live = 2 and Adjustments = Before + 2
                and Resources = 1 and Original.Data.References = 2);
            Guards.Read (Guard_Buffer, Original);
            Check (Live = 2 and Resources = 1 and Original.Data.References = 2);
            Guard_Buffer (1) := 1;
            Failed := False;
            begin
                Guards.Read (Guard_Buffer, Copy);
            exception
                when Guards.Data_Error => Failed := True;
            end;
            Check (Failed and Live = 2 and Resources = 1);
            Guard_Buffer (1) := 0;
            Fail_Adjust := True;
            Failed := False;
            begin
                Guards.Read (Guard_Buffer, Copy);
            exception
                when Program_Error => Failed := True;
            end;
            Fail_Adjust := False;
            Check (Failed and Live = 2 and Resources = 1 and Original.Data.References = 2);
        end;
        Check (Live = 0 and Resources = 0);
    end Run;
    procedure Dynamic_Check (Count : Positive) is
        type Row is array (1 .. Count) of Integer;
        type Table is array (1 .. 2) of Row;
        type Envelope is record
            Header : Storage_Element;
            Data : Table;
        end record;
        package IO is new Ada.Storage_IO (Envelope);
        Buffer : IO.Buffer_Type;
        A : Envelope := (Header => 99, Data => (others => (others => 42)));
        B : Envelope := (Header => 0, Data => (others => (others => 0)));
        subtype Packed_Text is String (1 .. Count);
        package Text_IO is new Ada.Storage_IO (Packed_Text);
        Text_Buffer : Text_IO.Buffer_Type;
        Text : Packed_Text := (others => 'Q');
    begin
        Check (IO.Buffer_Size = Envelope'Max_Size_In_Storage_Elements);
        IO.Write (Buffer, A);
        IO.Read (Buffer, B);
        Check (B.Header = 99 and B.Data (2) (Count) = 42);
        Text_IO.Write (Text_Buffer, Text);
        Text := (others => 'Z');
        Text_IO.Read (Text_Buffer, Text);
        Check (Text (1) = 'Q' and Text (Count) = 'Q');
    end Dynamic_Check;
    protected Results is
        procedure Done;
        function Count return Natural;
    private
        Value : Natural := 0;
    end Results;
    protected body Results is
        procedure Done is
        begin
            Value := Value + 1;
        end Done;
        function Count return Natural is
        begin
            return Value;
        end Count;
    end Results;
    task type Worker;
    task body Worker is
    begin
        Run;
        Dynamic_Check (3);
        Results.Done;
    end Worker;
begin
    Run;
    Dynamic_Check (1);
    Dynamic_Check (7);
    Dynamic_Check (70000);
    declare
        Workers : array (1 .. 4) of Worker;
    begin
        null;
    end;
    Check (Results.Count = 4);
    Put_Line ("storage io ok");
end StorageIO;
