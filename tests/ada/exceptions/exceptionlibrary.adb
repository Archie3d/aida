with Ada.Exceptions; use Ada.Exceptions;
with Ada.Streams.Stream_IO;
with Ada.Text_IO; use Ada.Text_IO;
with Ada.Unchecked_Deallocation;
procedure ExceptionLibrary is
    package Model is
        Failure : exception;
        Alias_Error : exception renames Failure;
    end Model;
    function Capture return Exception_Occurrence is
    begin
        raise Model.Failure with "factory message";
    exception
        when E : others =>
            return Result : Exception_Occurrence do
                Save_Occurrence (Result, E);
            end return;
    end Capture;
    function Forward return Exception_Occurrence is
    begin
        return Capture;
    end Forward;
    function Read_Back (File : Ada.Streams.Stream_IO.File_Type) return Exception_Occurrence is
    begin
        return Exception_Occurrence'Input (Ada.Streams.Stream_IO.Stream (File));
    end Read_Back;
    procedure Free is new Ada.Unchecked_Deallocation (Exception_Occurrence, Exception_Occurrence_Access);
    File : Ada.Streams.Stream_IO.File_Type;
    Heap : Exception_Occurrence_Access;
    Saved : Exception_Occurrence := Forward;
    Name : constant String := "EXCEPTIONLIBRARY.MODEL.FAILURE";
    Wide_Name : Wide_String := Wide_Exception_Name (Model.Alias_Error'Identity);
    Wider_Name : Wide_Wide_String := Wide_Wide_Exception_Name (Saved);
    Message : String (7 .. 5006) := (others => 'm');
    Channel : Ada.Streams.Stream_IO.Stream_Access;
    Wide_Value : Wide_Character := Wide_Character'Val (16#1234#);
    Wider_Value : Wide_Wide_Character := Wide_Wide_Character'Val (16#12345#);
    type Letter is new Character;
    for Letter'Size use 32;
    type Padded_Text is array (Positive range <>) of Letter;
    Text : Padded_Text := "ok";
begin
    if Wide_Character'Pos (Wide_Value) /= 16#1234#
        or Wide_Wide_Character'Pos (Wider_Value) /= 16#12345#
        or Wide_Character'Size /= 16 or Wide_Wide_Character'Size /= 32
        or Text (1) /= 'o' or Text (2) /= 'k' then raise Program_Error; end if;
    if Exception_Name (Saved) /= Name or Exception_Message (Saved) /= "factory message"
        or Wide_Name /= "EXCEPTIONLIBRARY.MODEL.FAILURE"
        or Wider_Name /= "EXCEPTIONLIBRARY.MODEL.FAILURE"
        or Wide_Name'First /= 1 or Wider_Name'First /= 1 then raise Program_Error; end if;
    Message (15) := Character'Val (0);
    Ada.Streams.Stream_IO.Create (File, Ada.Streams.Stream_IO.Out_File, "exceptionlibrary.tmp");
    Channel := Ada.Streams.Stream_IO.Stream (File);
    begin
        raise Model.Failure with Message;
    exception
        when E : others =>
            Write_Exception_Occurrence (Channel, E);
            Exception_Occurrence'Output (Channel, E);
    end;
    Exception_Occurrence'Output (Channel, Null_Occurrence);
    Ada.Streams.Stream_IO.Close (File);
    Ada.Streams.Stream_IO.Open (File, Ada.Streams.Stream_IO.In_File, "exceptionlibrary.tmp");
    Channel := Ada.Streams.Stream_IO.Stream (File);
    Read_Exception_Occurrence (Channel, Saved);
    if Exception_Identity (Saved) /= Model.Failure'Identity or Exception_Message (Saved) /= Message then
        raise Program_Error with "stream read lost identity or message";
    end if;
    Heap := new Exception_Occurrence'(Read_Back (File));
    Ada.Streams.Stream_IO.Delete (File);
    if Exception_Message (Heap.all) /= Message then raise Program_Error; end if;
    begin
        Reraise_Occurrence (Heap.all);
        raise Program_Error;
    exception
        when E : Model.Failure =>
            if Exception_Message (E) /= Message then raise Program_Error; end if;
    end;
    Free (Heap);
    Ada.Streams.Stream_IO.Create (File, Ada.Streams.Stream_IO.Out_File, "exceptionlibrary.tmp");
    Channel := Ada.Streams.Stream_IO.Stream (File);
    Exception_Occurrence'Output (Channel, Null_Occurrence);
    Ada.Streams.Stream_IO.Close (File);
    Ada.Streams.Stream_IO.Open (File, Ada.Streams.Stream_IO.In_File, "exceptionlibrary.tmp");
    Read_Exception_Occurrence (Ada.Streams.Stream_IO.Stream (File), Saved);
    if Exception_Identity (Saved) /= Null_Id then raise Program_Error; end if;
    Ada.Streams.Stream_IO.Delete (File);
    for I in 1 .. 4 loop
        begin
            case I is
                when 1 => Wide_Name := Wide_Exception_Name (Null_Id);
                when 2 => Wider_Name := Wide_Wide_Exception_Name (Null_Id);
                when 3 => Wide_Name := Wide_Exception_Name (Null_Occurrence);
                when others => Wider_Name := Wide_Wide_Exception_Name (Null_Occurrence);
            end case;
            raise Program_Error;
        exception
            when Constraint_Error => null;
        end;
    end loop;
    Put_Line ("exceptionlibrary: passed");
end ExceptionLibrary;
