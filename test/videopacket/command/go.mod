module git.code.oa.com/trpc-go/trpc-codec/videopacket/examples/command

go 1.13

require (
	git.code.oa.com/jce/jce v1.0.1
	git.code.oa.com/trpc-go/trpc-codec/videopacket v0.1.5
	git.code.oa.com/trpc-go/trpc-go v0.3.5
	git.code.oa.com/videocommlib/videopacket-go v1.0.2
)

replace git.code.oa.com/trpc-go/trpc-codec/videopacket => ../../

replace git.code.oa.com/trpc-go/trpc-codec/videopacket/examples/command/command_proto => ./command_proto
