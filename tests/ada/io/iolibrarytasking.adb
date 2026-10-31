with Ada.Text_IO; use Ada.Text_IO;
with Ada.Text_IO.Integer_IO;
with Ada.Text_IO.Float_IO;
with Ada.Sequential_IO;
with Ada.Direct_IO;
with Ada.Streams.Stream_IO;
procedure IOLibraryTasking is
    package I is new Ada.Text_IO.Integer_IO (Integer);
    package R is new Ada.Text_IO.Float_IO (Float);
    package S is new Ada.Sequential_IO (Integer);
    package D is new Ada.Direct_IO (Integer);
    package T renames Ada.Streams.Stream_IO;
    protected results is
        procedure done;
        function count return Natural;
    private
        finished : Natural := 0;
    end results;
    protected body results is
        procedure done is
        begin
            finished := finished + 1;
        end done;
        function count return Natural is
        begin
            return finished;
        end count;
    end results;
    task type Worker;
    task body Worker is
        value : Integer;
        realValue : Float;
    begin
        for iteration in 1 .. 50 loop
            declare
                text : File_Type;
                seq : S.File_Type;
                direct : D.File_Type;
                stream : T.File_Type;
            begin
                Create (text);
                S.Create (seq);
                D.Create (direct);
                T.Create (stream);
                Put_Line (text, " 16#FF#-12 .125");
                Reset (text, In_File);
                I.Get (text, value);
                if value /= 255 then
                    raise Program_Error;
                end if;
                I.Get (text, value);
                R.Get (text, realValue);
                if value /= -12 or realValue /= 0.125 or Col (text) /= 16 then
                    raise Program_Error;
                end if;
                if iteration mod 2 = 0 then
                    raise Constraint_Error;
                end if;
            exception
                when Constraint_Error => null;
            end;
        end loop;
        results.done;
    end Worker;
begin
    declare
        workers : array (1 .. 4) of Worker;
    begin
        null;
    end;
    if results.count /= 4 then
        raise Program_Error;
    end if;
    Put_Line ("iolibrarytasking: passed");
end IOLibraryTasking;
