package copyutils

// Copier is used to make a deepcopy if receiver implements it.
// The copy may share part of data with original one.
type Copier interface {
	Copy() (interface{}, error)
}

// CopierTo is used to make a shallow copy if receiver implements it.
// It may be used to keep address of some fields in dst unchanged.
type CopierTo interface {
	CopyTo(dst interface{}) error
}
