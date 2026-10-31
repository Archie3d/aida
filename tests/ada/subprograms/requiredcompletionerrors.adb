procedure RequiredCompletionErrors is
    package Missing is
        procedure Action;
    end Missing;
    package Wrong is
        procedure Action (Value : Integer);
    end Wrong;
    package body Wrong is
        procedure Action (Value : Boolean) is null;
    end Wrong;
    package body Orphan is
    end Orphan;
    package Twice is end Twice;
    package body Twice is end Twice;
    package body Twice is end Twice;
begin
    null;
end RequiredCompletionErrors;
