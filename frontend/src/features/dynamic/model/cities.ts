// 內嵌中國主要城市列表，用於天氣動態內容城市選擇器。
// locationId 傳給 QWeather。直轄市用穩定 location id，其餘城市用中文名交給 QWeather 解析。

export interface City {
  name: string;
  province: string;
  locationId: string;
}

export const CITIES: City[] = [
  // ── 直轄市 ──────────────────────────────────────────────────
  { name: '北京', province: '北京', locationId: '101010100' },
  { name: '天津', province: '天津', locationId: '101030100' },
  { name: '上海', province: '上海', locationId: '101020100' },
  { name: '重慶', province: '重慶', locationId: '101040100' },

  // ── 東北 ──────────────────────────────────────────────────
  { name: '哈爾濱', province: '黑龍江', locationId: '哈爾濱' },
  { name: '大慶', province: '黑龍江', locationId: '大慶' },
  { name: '齊齊哈爾', province: '黑龍江', locationId: '齊齊哈爾' },
  { name: '牡丹江', province: '黑龍江', locationId: '牡丹江' },
  { name: '長春', province: '吉林', locationId: '長春' },
  { name: '吉林市', province: '吉林', locationId: '吉林市' },
  { name: '瀋陽', province: '遼寧', locationId: '瀋陽' },
  { name: '大連', province: '遼寧', locationId: '大連' },
  { name: '鞍山', province: '遼寧', locationId: '鞍山' },

  // ── 華北 ──────────────────────────────────────────────────
  { name: '呼和浩特', province: '內蒙古', locationId: '呼和浩特' },
  { name: '包頭', province: '內蒙古', locationId: '包頭' },
  { name: '鄂爾多斯', province: '內蒙古', locationId: '鄂爾多斯' },
  { name: '石家莊', province: '河北', locationId: '石家莊' },
  { name: '唐山', province: '河北', locationId: '唐山' },
  { name: '保定', province: '河北', locationId: '保定' },
  { name: '邯鄲', province: '河北', locationId: '邯鄲' },
  { name: '太原', province: '山西', locationId: '太原' },
  { name: '大同', province: '山西', locationId: '大同' },

  // ── 西北 ──────────────────────────────────────────────────
  { name: '西安', province: '陝西', locationId: '西安' },
  { name: '咸陽', province: '陝西', locationId: '咸陽' },
  { name: '寶雞', province: '陝西', locationId: '寶雞' },
  { name: '蘭州', province: '甘肅', locationId: '蘭州' },
  { name: '銀川', province: '寧夏', locationId: '銀川' },
  { name: '烏魯木齊', province: '新疆', locationId: '烏魯木齊' },
  { name: '西寧', province: '青海', locationId: '西寧' },
  { name: '拉薩', province: '西藏', locationId: '拉薩' },

  // ── 華東 ──────────────────────────────────────────────────
  { name: '濟南', province: '山東', locationId: '濟南' },
  { name: '青島', province: '山東', locationId: '青島' },
  { name: '煙台', province: '山東', locationId: '煙台' },
  { name: '臨沂', province: '山東', locationId: '臨沂' },
  { name: '淄博', province: '山東', locationId: '淄博' },
  { name: '濰坊', province: '山東', locationId: '濰坊' },
  { name: '濟寧', province: '山東', locationId: '濟寧' },
  { name: '南京', province: '江蘇', locationId: '南京' },
  { name: '蘇州', province: '江蘇', locationId: '蘇州' },
  { name: '無錫', province: '江蘇', locationId: '無錫' },
  { name: '南通', province: '江蘇', locationId: '南通' },
  { name: '常州', province: '江蘇', locationId: '常州' },
  { name: '徐州', province: '江蘇', locationId: '徐州' },
  { name: '揚州', province: '江蘇', locationId: '揚州' },
  { name: '鎮江', province: '江蘇', locationId: '鎮江' },
  { name: '連雲港', province: '江蘇', locationId: '連雲港' },
  { name: '合肥', province: '安徽', locationId: '合肥' },
  { name: '蕪湖', province: '安徽', locationId: '蕪湖' },
  { name: '蚌埠', province: '安徽', locationId: '蚌埠' },
  { name: '杭州', province: '浙江', locationId: '杭州' },
  { name: '寧波', province: '浙江', locationId: '寧波' },
  { name: '温州', province: '浙江', locationId: '温州' },
  { name: '紹興', province: '浙江', locationId: '紹興' },
  { name: '嘉興', province: '浙江', locationId: '嘉興' },
  { name: '金華', province: '浙江', locationId: '金華' },
  { name: '台州', province: '浙江', locationId: '台州' },
  { name: '湖州', province: '浙江', locationId: '湖州' },
  { name: '福州', province: '福建', locationId: '福州' },
  { name: '廈門', province: '福建', locationId: '廈門' },
  { name: '泉州', province: '福建', locationId: '泉州' },
  { name: '南昌', province: '江西', locationId: '南昌' },
  { name: '贛州', province: '江西', locationId: '贛州' },

  // ── 華中 ──────────────────────────────────────────────────
  { name: '鄭州', province: '河南', locationId: '鄭州' },
  { name: '洛陽', province: '河南', locationId: '洛陽' },
  { name: '開封', province: '河南', locationId: '開封' },
  { name: '新鄉', province: '河南', locationId: '新鄉' },
  { name: '武漢', province: '湖北', locationId: '武漢' },
  { name: '宜昌', province: '湖北', locationId: '宜昌' },
  { name: '襄陽', province: '湖北', locationId: '襄陽' },
  { name: '長沙', province: '湖南', locationId: '長沙' },
  { name: '株洲', province: '湖南', locationId: '株洲' },
  { name: '岳陽', province: '湖南', locationId: '岳陽' },
  { name: '常德', province: '湖南', locationId: '常德' },

  // ── 華南 ──────────────────────────────────────────────────
  { name: '廣州', province: '廣東', locationId: '廣州' },
  { name: '深圳', province: '廣東', locationId: '深圳' },
  { name: '珠海', province: '廣東', locationId: '珠海' },
  { name: '佛山', province: '廣東', locationId: '佛山' },
  { name: '東莞', province: '廣東', locationId: '東莞' },
  { name: '中山', province: '廣東', locationId: '中山' },
  { name: '汕頭', province: '廣東', locationId: '汕頭' },
  { name: '湛江', province: '廣東', locationId: '湛江' },
  { name: '惠州', province: '廣東', locationId: '惠州' },
  { name: '南寧', province: '廣西', locationId: '南寧' },
  { name: '柳州', province: '廣西', locationId: '柳州' },
  { name: '海口', province: '海南', locationId: '海口' },
  { name: '三亞', province: '海南', locationId: '三亞' },

  // ── 西南 ──────────────────────────────────────────────────
  { name: '成都', province: '四川', locationId: '成都' },
  { name: '綿陽', province: '四川', locationId: '綿陽' },
  { name: '貴陽', province: '貴州', locationId: '貴陽' },
  { name: '昆明', province: '雲南', locationId: '昆明' },
];
