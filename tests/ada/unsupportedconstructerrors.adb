procedure UnsupportedConstructErrors is
    package Abstract_Operations is
        type Root is abstract tagged null record;
        procedure Operation (Self : Root) is abstract;
    end Abstract_Operations;
    task Worker;
    task type Worker_Type;
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
