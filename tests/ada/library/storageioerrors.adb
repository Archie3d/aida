with Ada.Storage_IO;
with Ada.Finalization;
procedure StorageIOErrors is
    package Open_String is new Ada.Storage_IO (String);
    type Limited_Item is limited record
        Value : Integer;
    end record;
    package Limited_IO is new Ada.Storage_IO (Limited_Item);
    type Root is tagged null record;
    package Class_IO is new Ada.Storage_IO (Root'Class);
    type Sized (N : Natural := 1) is record
        Data : String (1 .. N);
    end record;
    package Dependent_IO is new Ada.Storage_IO (Sized);
    task type Job;
    package Task_IO is new Ada.Storage_IO (Job);
    task body Job is begin null; end Job;
    protected type Lock is
        procedure Touch;
    end Lock;
    package Protected_IO is new Ada.Storage_IO (Lock);
    protected body Lock is
        procedure Touch is begin null; end Touch;
    end Lock;
    package Integers is new Ada.Storage_IO (Integer);
    B : constant Integers.Buffer_Type := (others => 0);
    X : constant Integer := 0;
    procedure Wrong (Buffer : in Integers.Buffer_Type; Item : in Integer) renames Integers.Read;
    type Open_Array is array (Integer range <>) of Integer;
    Bad_Size : constant := Open_Array'Max_Size_In_Storage_Elements;
    Item : Integer := 0;
    Object_Size : constant := Item'Max_Size_In_Storage_Elements;
begin
    Integers.Write (B, 1);
    Integers.Read (B, X);
end StorageIOErrors;
