procedure UncheckedSharedErrors is
    type Link is access all Integer;
    Shared : aliased Integer := 0;
    pragma Atomic (Shared);
    P : Link := Shared'Unchecked_Access;
begin
    null;
end UncheckedSharedErrors;
