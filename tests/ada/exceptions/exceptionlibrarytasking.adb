with Ada.Exceptions; use Ada.Exceptions;
with Ada.Streams; use Ada.Streams;
with Ada.Text_IO; use Ada.Text_IO;
procedure ExceptionLibraryTasking is
    Failure : exception;
    procedure Scenario is
        type Memory_Stream is new Root_Stream_Type with record
            Bytes : Stream_Element_Array (1 .. 4096);
            Used : Stream_Element_Offset := 0;
            Position : Stream_Element_Offset := 1;
        end record;
        overriding procedure Read (Stream : in out Memory_Stream;
                                   Item : out Stream_Element_Array;
                                   Last : out Stream_Element_Offset);
        overriding procedure Write (Stream : in out Memory_Stream;
                                    Item : Stream_Element_Array);
        overriding procedure Read (Stream : in out Memory_Stream;
                                   Item : out Stream_Element_Array;
                                   Last : out Stream_Element_Offset) is
        begin
            Last := Item'First - 1;
            for I in Item'Range loop
                exit when Stream.Position > Stream.Used;
                Item (I) := Stream.Bytes (Stream.Position);
                Stream.Position := Stream.Position + 1;
                Last := I;
            end loop;
        end Read;
        overriding procedure Write (Stream : in out Memory_Stream;
                                    Item : Stream_Element_Array) is
        begin
            for I in Item'Range loop
                Stream.Used := Stream.Used + 1;
                Stream.Bytes (Stream.Used) := Item (I);
            end loop;
        end Write;
        function Back (Stream : not null access Root_Stream_Type'Class) return Exception_Occurrence is
        begin
            return Exception_Occurrence'Input (Stream);
        end Back;
        Buffer : aliased Memory_Stream;
        Message : constant String (1 .. 1000) := (others => 'z');
    begin
        for I in 1 .. 20 loop
            Buffer.Used := 0;
            Buffer.Position := 1;
            begin
                raise Failure with Message;
            exception
                when E : Failure => Exception_Occurrence'Output (Buffer'Access, E);
            end;
            declare
                Saved : Exception_Occurrence := Back (Buffer'Access);
            begin
                if Exception_Identity (Saved) /= Failure'Identity or Exception_Message (Saved) /= Message then
                    raise Program_Error;
                end if;
                begin
                    Reraise_Occurrence (Saved);
                exception
                    when E : Failure =>
                        if Exception_Message (E) /= Message then raise Program_Error; end if;
                end;
            end;
        end loop;
    end Scenario;
begin
    declare
        task type Worker;
        task body Worker is
        begin
            Scenario;
        exception
            when others => Put_Line ("exception worker failed");
        end Worker;
        Workers : array (1 .. 4) of Worker;
    begin
        null;
    end;
    Put_Line ("exceptionlibrarytasking: passed");
end ExceptionLibraryTasking;
