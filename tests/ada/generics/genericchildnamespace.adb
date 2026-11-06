procedure GenericChildNamespace is
    generic
    package Conflict is
    end Conflict;
    package Conflict is
    end Conflict;
begin
    null;
end GenericChildNamespace;
