procedure UnsupportedConstructErrors is
    package Abstract_Operations is
        type Root is abstract tagged null record;
        procedure Operation (Self : Root) is abstract;
    end Abstract_Operations;
    task Worker is
        entry Run;
    end Worker;
    task type Worker_Type (Id : Integer);
begin
    goto Target;
    <<Target>>
    null;
    select
        delay 0.0;
    then abort
        null;
    end select;
end UnsupportedConstructErrors;
