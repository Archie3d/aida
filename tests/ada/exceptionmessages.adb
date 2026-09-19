with Ada.Text_IO; use Ada.Text_IO;
with Ada.Exceptions; use Ada.Exceptions;

procedure Exceptionmessages is
    Original, Other : exception;
    Alias_Error : exception renames Original;
    Empty : Exception_Occurrence;
    Long_Message : String (5 .. 5004) := (others => 'x');
    Count : Integer := 0;

    procedure Check (Condition : Boolean) is
    begin
        if not Condition then
            raise Program_Error with "message regression failed";
        end if;
    end Check;

    procedure Throw is
        Text : String := "owned " & "message";
    begin
        raise Original with Text;
    end Throw;

    procedure Throw_Again is
    begin
        Throw;
    exception
        when E : Original =>
            Check (Exception_Identity (E) = Original'Identity);
            Check (Exception_Name (E) = "ORIGINAL");
            Check (Exception_Information (E) (1 .. 23) = "ORIGINAL: owned message");
            declare
                procedure Captured is
                begin
                    Check (Exception_Message (E) = "owned message");
                end Captured;
            begin
                begin
                    raise Other with "inner message";
                exception
                    when F : others =>
                        Check (Exception_Message (F) = "inner message");
                        Captured;
                end;
                Captured;
            end;
            raise;
    end Throw_Again;

    function Failed_Message return String is
    begin
        raise Other with "message evaluation failed";
        return "unreachable";
    end Failed_Message;
begin
    Check (Original'Identity = Alias_Error'Identity);
    Check (Exception_Name (Original'Identity) = "ORIGINAL");
    Check (Exception_Identity (Empty) = Null_Id);
    Check (Exception_Identity (Null_Occurrence) = Null_Id);
    Reraise_Occurrence (Null_Occurrence);
    for I in 1 .. 3 loop
        begin
            Throw_Again;
        exception
            when E : others =>
                Check (Exception_Message (E) = "owned message");
                Check (Exception_Identity (E) = Original'Identity);
                Count := Count + 1;
        end;
    end loop;
    Check (Count = 3);
    -- Long messages, non-1 lower bounds, and embedded NUL survive unchanged.
    Long_Message (2500) := Character'Val (0);
    begin
        begin
            Raise_Exception (Original'Identity, Long_Message);
        exception
            when E : others => Reraise_Occurrence (E);
        end;
    exception
        when E : Original =>
            declare
                Saved : String := Exception_Message (E);
            begin
                Check (Saved'First = 1 and Saved'Length = 5000);
                Check (Saved = Long_Message);
                for I in 1 .. 12 loop
                    Check (Exception_Message (E) = Saved);
                end loop;
            end;
    end;
    -- Unnamed handlers must retain the message for a bare re-raise too.
    begin
        begin
            raise Original with "unnamed";
        exception
            when Original => raise;
        end;
    exception
        when E : others => Check (Exception_Message (E) = "unnamed");
    end;
    begin
        raise Original with Failed_Message;
    exception
        when E : Other => Check (Exception_Message (E) = "message evaluation failed");
    end;
    begin
        Raise_Exception (Original'Identity);
    exception
        when E : Original =>
            Check (Exception_Message (E) = "");
            Check (Exception_Information (E) (1 .. 8) = "ORIGINAL");
    end;
    begin
        raise Original;
    exception
        when E : Original => Check (Exception_Message (E) = "");
    end;
    -- Null occurrence inspection has the specified Constraint_Error behavior.
    for I in 1 .. 5 loop
        begin
            case I is
                when 1 => Put_Line (Exception_Name (Null_Id));
                when 2 => Put_Line (Exception_Name (Null_Occurrence));
                when 3 => Put_Line (Exception_Message (Empty));
                when 4 => Put_Line (Exception_Information (Empty));
                when 5 => Raise_Exception (Null_Id, "ignored");
                when others => raise Program_Error;
            end case;
            raise Program_Error;
        exception
            when E : Constraint_Error => Check (Exception_Message (E) = "");
        end;
    end loop;
    Put_Line ("exception messages ok");
end Exceptionmessages;
