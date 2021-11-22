package videopacket

import (
	"errors"
	"fmt"

	"git.code.oa.com/jce/jce"
	"git.code.oa.com/trpc-go/trpc-go/codec"
	p "git.code.oa.com/videocommlib/videopacket-go"
)

// VideoPacketBodyPackSerializationType 用于注册序列化类型
// 业务序列化类型需要从1000开始，尽量分散与其他业务不冲突
const VideoPacketBodyPackSerializationType = 8001

func init() {
	codec.RegisterSerializer(VideoPacketBodyPackSerializationType, &videoPacketBodyPack{})
}

// videoPacketBodyPack 是一种比默认 jce 序列化方式 (参考 git.code.oa.com/jce/jce) 多一步操作。
// 序列化时放入 tag2 的 jce 结构里，反序列化时从 tag1 的 jce 结构里读取
// cpp 的spp 服务启动时是在读到包头时才知道采用默认的 rpc 方式（即默认jce序列化方式）还是 videoPacketBodyPack 的方式
// 判断根据是 head.AccessInfo.QUAInfo.extInfo["spp_rpc_video_packet_type"] = "1"
// 故需要在 server 端 codec 的 Encode 和 Decode 中使用
type videoPacketBodyPack struct{}

// Unmarshal 实现了 videoPacketBodyPack 的反序列方法
func (v *videoPacketBodyPack) Unmarshal(in []byte, body interface{}) error {
	if _, ok := body.(jce.Message); !ok {
		return errors.New("videopacket body pack unmarshal with err: body not jce.Messaage")
	}
	jceReader := jce.NewReader(in)
	if err, _ := jceReader.SkipTo(jce.STRUCT_BEGIN, 1, false); err != nil {
		return fmt.Errorf("videopacket body pack unmarshal skip to struct begin with err: %v", err)
	}
	if err := body.(jce.Message).ReadFrom(jceReader); err != nil {
		return fmt.Errorf("videopacket body pack unmarshal body read from with err: %v", err)
	}
	if err := jceReader.SkipToStructEnd(); err != nil {
		return fmt.Errorf("videopacket body pack unmarshal skip to struct end with err: %v", err)
	}
	return nil
}

// Marshal 实现了 videoPacketBodyPack 的序列化方法
func (v *videoPacketBodyPack) Marshal(body interface{}) (out []byte, err error) {
	if _, ok := body.(jce.Message); !ok {
		return nil, errors.New("videopacket body pack marshal with err: body not jce.Messaage")
	}
	jceWriter := jce.NewBuffer()
	err = jceWrite(jceWriter, body)
	if err != nil {
		return nil, err
	}
	return jceWriter.ToBytes(), nil
}

// jceWrite jce write
func jceWrite(jceWriter *jce.Buffer, body interface{}) error {
	if err := jceWriter.Write_int32(0, 0); err != nil {
		return fmt.Errorf("videopacket body pack masrshal write tag 0 with 0 err: %s", err.Error())
	}
	if err := jceWriter.WriteHead(jce.STRUCT_BEGIN, 2); err != nil {
		return fmt.Errorf("videopacket body pack marshal write head struct begin with err: %s", err.Error())
	}
	if err := body.(jce.Message).WriteTo(jceWriter); err != nil {
		return fmt.Errorf("videopacket body pack marshal write to body with err: %s", err.Error())
	}
	if err := jceWriter.WriteHead(jce.STRUCT_END, 0); err != nil {
		return fmt.Errorf("videopacket body pack marshal write head struct end with err: %s", err.Error())
	}
	return nil
}

// needBodyPack 返回是否需要 body pack
func needBodyPack(head *p.VideoPacket) bool {
	if head == nil {
		return false
	}
	if head.CommHeader.AccessInfo.QUAInfo.ExtInfo["spp_rpc_video_packet_type"] == "1" {
		return true
	}
	return false
}
