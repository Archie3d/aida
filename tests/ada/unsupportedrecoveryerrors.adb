limited private with Ada.Text_IO;
procedure UnsupportedRecoveryErrors is
    function Abstract_Function return Integer is abstract;
    task type Worker (First : Integer; Last : Integer) is
        entry Work (Left : Integer; Right : Integer);
    end Worker;
    task Single_Worker is
        entry Work;
    end;
    Value : Integer := 0;
begin
    <<First_Label>>
    <<Second_Label>>
    goto Destination;
    select
        select
            delay 1.0;
        then abort
            null;
        end select;
    or
        delay 2.0;
    end select;
    goto Destination;
    Value := 1;
end UnsupportedRecoveryErrors;
